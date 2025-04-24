#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>

#define temp1_enb A3
#define temp2_enb A1
#define temp3_enb 5
#define temp4_enb 6
#define temp1_adc A5
#define temp2_adc A2
#define temp3_adc A0
#define temp4_adc 8
#define opv_enb 13
#define ONE_WIRE_BUS 9
// Definisci i pin per RS485
#define RS485_TX_PIN 1   // Pin TX di Arduino (pin 1 per Arduino Uno)
#define RS485_RX_PIN 0   // Pin RX di Arduino (pin 0 per Arduino Uno)

SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);  // Imposta la comunicazione seriale RS485

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress; // Indirizzi delle sonde
uint8_t numSensors = 0; // Numero di sonde trovate
const byte samples = 5; //Numero di misurazioni, il risultato finale viene quindi mediato dal numero, maggiore precisione
bool temp_active[4];
uint16_t temp_value[16];

void setup() {
  Serial.begin(9600);
  pinMode(temp1_enb, OUTPUT);
  pinMode(temp2_enb, OUTPUT);
  pinMode(temp3_enb, OUTPUT);
  pinMode(temp4_enb, OUTPUT);
  pinMode(opv_enb, OUTPUT);
  sensors.begin(); // Inizializza i DS18B20
  numSensors = sensors.getDeviceCount(); // Conta le sonde disponibili
  if (numSensors > 12) {
    numSensors = 16; // Limite massimo per evitare overflow
  } else{
    numSensors = numSensors + 4;
  }
  
  for (byte i = 0; i < 4; i++) {
    temp_active[i] = 1;
  }
  // Inizializza la comunicazione seriale RS485
  rs485.begin(9600);  // Imposta la velocità di comunicazione RS485
}

void opv_getvalue() {//---------- Misurare la tensione della temperatura ----------
  digitalWrite(opv_enb, 1); //Abilita Opv
  //Attiva i canali, come in Impostazioni
  digitalWrite(temp1_enb, temp_active[0]);
  digitalWrite(temp2_enb, temp_active[1]);
  digitalWrite(temp3_enb, temp_active[2]);
  digitalWrite(temp4_enb, temp_active[3]);
  delay(15);

  //Assegnare il canale corrente e attivo alle variabili ausiliarie, ciò è possibile con un ciclo for
  for (byte i = 0; i < 4; i++) {
    temp_value[i] = 0;
    int pin = 0;
    int pin_read = 0;
    if (temp_active[i] == 1) {

      switch (i) {
        case 0:
          pin = temp1_enb;
          pin_read = temp1_adc;
          break;
        case 1:
          pin = temp2_enb;
          pin_read = temp2_adc;
          break;
        case 2:
          pin = temp3_enb;
          pin_read = temp3_adc;
          break;
        case 3:
          pin = temp4_enb;
          pin_read = temp4_adc;
          break;
      }
      for(byte j = 0; j < samples; j++){
        temp_value[i] += analogRead(pin_read); //Voltaggio nella migliore delle ipotesi. Misurare il canale e convertirlo in temperatura
        delay(200);
      }
      temp_value[i] =  (((temp_value[i]/samples) * 0.153479) - 27.23) * 10; //Converti il ​​valore ADC a 10 bit in temp
      digitalWrite(pin, 0); //Disattiva canale
    }
  }

  digitalWrite(opv_enb, 0); //Disabilita OPV
}

void loop() {
  opv_getvalue();
  sensors.requestTemperatures(); 
  for (int i = 4; i < numSensors; i++) {
    float tempC = sensors.getTempCByIndex(i);
    if (tempC == DEVICE_DISCONNECTED_C) {
        tempC = -100; // Errore: sonda non risponde
    }
    temp_value[i] = (uint16_t)(tempC * 10); // Conversione in intero
  }

  Serial.print("Temperature: ");
    for (int i = 0; i < numSensors; i++) {
        Serial.print(temp_value[i] / 10.0);
        Serial.print(" ");
    }
  Serial.println();

  for (int i = 0; i < numSensors; i++) {
    rs485.print(i);
    rs485.print("x");
    rs485.print(temp_value[i]);
    rs485.print("\n");
    rs485.flush();  // Assicurati che i dati vengano inviati
  }

  delay(2000);
}

