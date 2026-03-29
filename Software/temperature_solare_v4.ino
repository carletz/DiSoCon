/**
 * temperature_solare_v3.ino
 * Lettura 4 canali PT100/PT1000 via OPV → ADC ATmega32U4
 * Trasmissione via RS485 (Serial1, UART hardware del 32U4)
 *
 * Hardware: ATmega32U4 (Arduino Leonardo / Micro / PCB custom)
 *
 * Lettura OneWire DS18B20: commentata, non usata in questa versione.
 */

// #include <OneWire.h>           // -- DS18B20: non usato --
// #include <DallasTemperature.h> // -- DS18B20: non usato --

#include <avr/sleep.h>   // ADC Noise Reduction Mode
#include <avr/interrupt.h>

// ============================================================
//  PIN MAPPING  (ATmega32U4 / PCB custom)
//  Define originali verificate sul campo — non modificare.
// ============================================================

// Pin di enable per ogni canale
static const uint8_t ENB_PINS[4] = { A3, A1, 5, 6 };

// Delay di stabilizzazione per canale (ms) — aumentare per cavi lunghi o RC alto
// CH0: cavi lunghi → 80ms | altri: 20ms
static const uint8_t CHANNEL_SETTLE_MS[4] = { 80, 20, 20, 20 };

// Numero di campioni ADC per canale — più campioni = più robusto al rumore
// CH0: cavo lungo vicino a 230V → 60 campioni | altri: 20
static const uint8_t CHANNEL_SAMPLES[4] = { 60, 20, 20, 20 };

// Filtro IIR (media mobile esponenziale) per canale
// valore_filtrato = ALPHA * lettura_nuova + (1-ALPHA) * valore_precedente
// Alpha basso = filtro aggressivo (lento ma stabile)
// Alpha alto  = filtro leggero (veloce ma più rumoroso)
// CH0: 0.3 (compromesso tra reattività e stabilità) | altri: 0.5
static const float IIR_ALPHA[4] = { 0.5f, 0.5f, 0.5f, 0.5f };
static float iirFiltered[4] = { -999.0f, -999.0f, -999.0f, -999.0f }; // -999 = non inizializzato

// Pin ADC per ogni canale
//   ch3: pin 8 = PB4 = ADC11 sul 32U4, funzionante come analogRead(8)
static const uint8_t ADC_PINS[4] = { A5, A2, A0, 8 };

// Pin enable dello stadio OPV
static const uint8_t OPV_ENB_PIN = 13;

// ============================================================
//  RS485  –  ATmega32U4 ha due UART hardware:
//    Serial  = USB CDC  (debug / monitor seriale)
//    Serial1 = pin 0/1  (TX1/RX1) → transceiver RS485
//  Pin DE/RE del transceiver (HIGH = trasmissione, LOW = ricezione)
// ============================================================
static const uint8_t RS485_DE_PIN = 2; // Cambia se necessario; -1 = non usato

// ============================================================
//  CALIBRAZIONE PT1000  (circuito: MCP6004 + partitore 4.7kΩ, Vcc=3.3V)
// ============================================================
//
//  Amplificatore non-invertente: G = 1 + R_fb/R_gain = 1 + 9100/1000 = 10.1
//  Partitore di offset 4.7kΩ/4.7kΩ → V_offset ≈ 1.65V sull'ingresso inv.
//  La curva V_out(T) è monotona e quasi lineare nel range di lavoro,
//  quindi la conversione lineare è adeguata:
//
//    T[°C] = ADC_GAIN * adcMedian + ADC_OFFSET
//
//  ── Come calibrare a 2 punti ────────────────────────────────────────
//  1. Sonda a T1 nota (es. 25.0°C) → leggi adcMedian → ADC1
//  2. Sonda a T2 nota (es. 80.0°C) → leggi adcMedian → ADC2
//     ADC_GAIN   = (T2 - T1) / (float)(ADC2 - ADC1)
//     ADC_OFFSET = T1 - ADC_GAIN * ADC1
//
//  ── Valori attuali (verificati sul campo nel codice originale) ───────
// Coefficienti di calibrazione per canale
// Tutti i canali usano gli stessi coefficienti originali
// Per calibrare a 2 punti: GAIN=(T2-T1)/(ADC2-ADC1), OFFSET=T1-GAIN*ADC1
static const float CHAN_GAIN[4]   = { 0.153479f, 0.153479f, 0.153479f, 0.153479f };
static const float CHAN_OFFSET[4] = {   -27.23f,   -27.23f,   -27.23f,   -27.23f };

//  ── Correzione fine per canale (in decimi di °C) ────────────────────
//  Compensa disomogeneità tra i 4 canali (tolleranze R, offset OPV).
//  NOTA: il +50 su ch0 (+5.0°C) era nel codice originale — verificare
//  se è una correzione hardware reale o un residuo di debug.
// Correzione offset per canale (decimi di °C)
static const int16_t CHANNEL_TRIM[4] = { 0, 0, 0, 0 };

// ============================================================
//  RILEVAMENTO SONDA DISCONNESSA / IN CORTOCIRCUITO
// ============================================================
//
//  Con PT100 su partitore 5V:
//    – Circuito aperto  (sonda scollegata): ADC → 1023 (tensione massima)
//    – Cortocircuito    (PT → 0Ω):         ADC → 0
//  Definiamo soglie di validità con margine di sicurezza.
//
//  Calcola i tuoi limiti:
//    ADC_MIN = adcAvg a T_min operativa (es. -30°C) – margine
//    ADC_MAX = adcAvg a T_max operativa (es. +200°C) + margine
static const uint16_t ADC_FAULT_LOW  =  10;   // sotto → cortocircuito
static const uint16_t ADC_FAULT_HIGH = 1010;  // sopra → circuito aperto

// Valore sentinella per errore (in decimi di °C): 32767 = +3276.7°C
// Valore positivo inequivocabile → il master lo interpreta come fault (> 30000)
static const int16_t TEMP_ERROR = 32767;

// ============================================================
//  CAMPIONAMENTO ADC con Noise Reduction Mode (ATmega32U4)
// ============================================================
//
//  Il processore entra in ADC Noise Reduction Sleep durante la conversione:
//  clock CPU, I/O digitali e altri moduli vengono silenziati, riducendo
//  il rumore sul bus analogico di ~2-4 LSB rispetto a analogRead() normale.
//
//  Dopo ADC_SAMPLES letture, i valori vengono ordinati e viene calcolata
//  la mediana (più robusta della media contro picchi di disturbo).

// Numero massimo di campioni possibile (dimensiona il buffer in adcMedian)
static const uint8_t ADC_SAMPLES_MAX = 60;

volatile bool adcDone = false;

// ISR risveglio dal sleep (basta uscire dal modo sleep)
ISR(ADC_vect) {
  adcDone = true;
}

/**
 * Esegue una singola conversione ADC in Noise Reduction Mode.
 * Restituisce il valore grezzo a 10 bit.
 */
static uint16_t adcReadNoiseFree(uint8_t pin) {
  // Usa analogRead() normale per impostare il MUX correttamente,
  // poi ripete la conversione in sleep mode per la lettura effettiva.
  analogRead(pin); // warm-up: imposta MUX e scarica capacità di campionamento

  adcDone = false;
  ADCSRA |= (1 << ADIE);          // abilita interrupt ADC
  set_sleep_mode(SLEEP_MODE_ADC); // Noise Reduction Mode
  sei();
  sleep_enable();
  sleep_cpu();                    // la CPU si ferma; l'ADC converte da solo
  sleep_disable();
  ADCSRA &= ~(1 << ADIE);         // disabilita interrupt ADC

  // Leggi risultato (ADCL va letto per primo per bloccare ADCH)
  uint16_t result = ADC;
  return result;
}

/**
 * Campiona il canale ADC `pin` per `n` volte, ordina i valori e
 * restituisce la mediana (resistente a picchi/glitch).
 */
static uint16_t adcMedian(uint8_t pin, uint8_t n) {
  uint16_t buf[ADC_SAMPLES_MAX]; // buffer dimensionato sul massimo possibile
  for (uint8_t i = 0; i < n; i++) {
    buf[i] = adcReadNoiseFree(pin);
    // Nessun delay: il Noise Reduction Mode già garantisce conversioni pulite.
    // Se il circuito ha RC lento, aggiungi delayMicroseconds(200) qui.
  }

  // Insertion sort (efficiente per n piccolo, niente heap dinamico)
  for (uint8_t i = 1; i < n; i++) {
    uint16_t key = buf[i];
    int8_t j = i - 1;
    while (j >= 0 && buf[j] > key) {
      buf[j + 1] = buf[j];
      j--;
    }
    buf[j + 1] = key;
  }

  // Mediana: media dei due centrali (n pari) per evitare bias
  return (buf[n / 2 - 1] + buf[n / 2]) / 2;
}

// ============================================================
//  DATI GLOBALI
// ============================================================

// temp_value[0..3]: temperature canali OPV in decimi di °C
//   Es.: 235 = 23.5°C | TEMP_ERROR (0x8000) = fault
static int16_t   temp_value[4];
static uint16_t  adcRawDebug[4]; // DEBUG: valori ADC grezzi per calibrazione
static uint32_t  lastReadTime = 0;
static const uint32_t READ_INTERVAL = 15000; // ms

// -- DS18B20: strutture commentate --
// OneWire          oneWire(9);
// DallasTemperature sensors(&oneWire);
// static int16_t   ds_temp[12];
// static uint8_t   numDS = 0;

// ============================================================
//  SETUP
// ============================================================

void setup() {
  // USB CDC per debug (Serial su 32U4 è virtuale, non usa pin fisici)
  Serial.begin(9600);

  // UART hardware Serial1 (pin 0=RX1, 1=TX1) per RS485
  Serial1.begin(9600);
  if (RS485_DE_PIN >= 0) {
    pinMode(RS485_DE_PIN, OUTPUT);
    digitalWrite(RS485_DE_PIN, HIGH); // sempre in TX (half-duplex)
  }

  // OPV e canali spenti all'avvio
  pinMode(OPV_ENB_PIN, OUTPUT);
  digitalWrite(OPV_ENB_PIN, LOW);
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(ENB_PINS[i], OUTPUT);
    digitalWrite(ENB_PINS[i], LOW);
  }

  // AVCC (pin 44) è collegato a 3V3 sullo schematico → referenza ADC = 3.3V
  // AREF (pin 42) ha solo il condensatore di bypass → non usare EXTERNAL.
  analogReference(DEFAULT); // usa AVCC = 3V3

  // -- DS18B20: init commentato --
  // sensors.begin();
  // numDS = min(sensors.getDeviceCount(), 12);

  Serial.println(F("Sistema pronto."));
}

// ============================================================
//  LETTURA CANALI OPV
// ============================================================

void opv_readChannels() {
  digitalWrite(OPV_ENB_PIN, HIGH);
  delay(100); // attendi stabilizzazione alimentazione OPV (era 30ms, insufficiente)

  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(ENB_PINS[i], HIGH);
    delay(CHANNEL_SETTLE_MS[i]); // stabilizzazione RC: variabile per canale (cavi lunghi → più tempo)

    uint16_t adcVal = adcMedian(ADC_PINS[i], CHANNEL_SAMPLES[i]);
    adcRawDebug[i] = adcVal; // DEBUG: salva ADC grezzo per calibrazione

    digitalWrite(ENB_PINS[i], LOW);

    // ── Rilevamento fault ──────────────────────────────────
    if (adcVal <= ADC_FAULT_LOW) {
      temp_value[i] = TEMP_ERROR; // PT in cortocircuito
      iirFiltered[i] = -999.0f;  // reset filtro
      Serial.print(F("CH")); Serial.print(i); Serial.println(F(": CORTOCIRCUITO"));
      continue;
    }
    if (adcVal >= ADC_FAULT_HIGH) {
      temp_value[i] = TEMP_ERROR; // PT scollegata / circuito aperto
      iirFiltered[i] = -999.0f;  // reset filtro
      Serial.print(F("CH")); Serial.print(i); Serial.println(F(": SONDA ASSENTE"));
      continue;
    }

    // ── Conversione ADC → °C (PT100/PT1000 linearizzata) ──
    float tempC = CHAN_GAIN[i] * (float)adcVal + CHAN_OFFSET[i];

    // ── Filtro IIR ─────────────────────────────────────────
    if (iirFiltered[i] <= -998.0f) {
      iirFiltered[i] = tempC; // prima lettura: inizializza senza filtrare
    } else {
      iirFiltered[i] = IIR_ALPHA[i] * tempC + (1.0f - IIR_ALPHA[i]) * iirFiltered[i];
    }

    temp_value[i] = (int16_t)(iirFiltered[i] * 10.0f) + CHANNEL_TRIM[i];
  }

  digitalWrite(OPV_ENB_PIN, LOW);
}

// ============================================================
//  DEBUG SU USB SERIAL
// ============================================================

void debug_print() {
  Serial.print(F("Temp [°C]: "));
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print(F("CH")); Serial.print(i); Serial.print('=');
    if (temp_value[i] == TEMP_ERROR) {
      Serial.print(F("FAULT"));
    } else {
      Serial.print(temp_value[i] / 10.0f, 1);
      Serial.print(F("°C"));
    }
    Serial.print(' ');
  }
  Serial.println();

  // DEBUG CALIBRAZIONE: stampa valori ADC grezzi
  Serial.print(F("ADC raw:   "));
  for (uint8_t i = 0; i < 4; i++) {
    // Ri-legge adcRaw tramite variabile esterna — vedi opv_readChannels
    Serial.print(F("CH")); Serial.print(i); Serial.print('=');
    Serial.print(adcRawDebug[i]);
    Serial.print(' ');
  }
  Serial.println();
}

// ============================================================
//  INVIO RS485  –  protocollo ASCII "<idx>x<decimi_C>\n"
// ============================================================
//  Il master legge registri 0..3.
//  TEMP_ERROR (0x8000 = -32768 come int16_t) segnala fault al master.

void rs485_send() {
  for (uint8_t i = 0; i < 4; i++) {
    Serial1.print(i);
    Serial1.print('x');
    Serial1.print(temp_value[i]);
    Serial1.print('\n');
  }
  Serial1.flush();
}

// ============================================================
//  LOOP
// ============================================================

void loop() {
  uint32_t now = millis();
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;

    opv_readChannels();
    debug_print();
    rs485_send();

    // -- DS18B20: blocco commentato --
    // sensors.requestTemperatures();
    // for (uint8_t i = 0; i < numDS; i++) {
    //   float t = sensors.getTempCByIndex(i);
    //   ds_temp[i] = (t == DEVICE_DISCONNECTED_C) ? TEMP_ERROR : (int16_t)(t * 10.0f);
    // }
  }
}
