# ☀️ DiSoCon2 — Differential Solar Controller V2

> **A fully open-source, ESP32-S3 based differential solar thermal controller with dual-tank switching, PT1000 sensing, TRIAC pump control, and Ethernet/MQTT connectivity.**

[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)
[![ESPHome](https://img.shields.io/badge/Firmware-ESPHome-brightgreen)](https://esphome.io)
[![Platform: ESP32-S3](https://img.shields.io/badge/MCU-ESP32--S3-red)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Version](https://img.shields.io/badge/Version-V2.0--2026-orange)](.)

---

## 🌡️ What Is This?

DiSoCon2 is a modular, open-hardware differential solar thermal controller designed to intelligently manage a domestic solar hot water system with **multiple storage tanks**. It replaces proprietary "dumb" controllers with a fully integrated, home-automation-aware solution built around the **ESP32-S3** and **ATmega32U4**.

The system reads four PT1000 platinum resistance sensors (solar collector, primary tank, secondary tank, return line), computes temperature differentials, and drives pumps and 3-way valves accordingly — all while reporting live data to **Home Assistant** via MQTT or **Ethernet**.

**V2 integrates the control logic and the analog temperature measurement circuitry onto a single board**, eliminating the inter-board wiring of V1 and improving signal integrity.

---

## 🎯 Key Features

| Feature | Detail |
|---|---|
| **Temperature sensing** | 4× PT1000 via precision OPV front-end (MCP6004, ±0.1% resistors) + DS18B20 OneWire bus |
| **Pump control** | 2× TRIAC outputs (BTA41-600B + MOC3021) for 230V AC pumps, with zero-cross synchronization |
| **Valve control** | 2× relay outputs (SRD-05VDC) for 3-way motorized valves, optoisolated via PC817C |
| **Connectivity** | W5500 Ethernet (SPI) + ESP32-S3 onboard WiFi/BLE |
| **Field bus** | RS-485 half-duplex via MAX13487E (auto-direction), with 120Ω termination and SMBJ6.8A TVS protection |
| **User interface** | 20×4 I2C LCD + 5-button keypad (UP / DOWN / LEFT / RIGHT / OK) read via ADS1115 |
| **Power supply** | HLK-PM01 mains-to-5V module + BL1117-33CX LDO for 3.3V logic; USB-C 5V input as alternative |
| **Safety** | Anti-freeze logic, overheating protection, fuse-protected AC input, snubber networks on all TRIAC outputs |
| **Firmware** | ESPHome YAML on ESP32-S3; ATmega32U4 Arduino firmware for analog front-end |

---

## 🧩 System Architecture

The project is split into two physically separate boards that connect via a 10-pin IDC ribbon:

```
┌─────────────────────────────────────────────────┐
│              CONTROL BOARD (Full Board V2)      │
│                                                 │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │ ATmega   │  │ ESP32-S3 │  │  W5500        │  │
│  │ 32U4     │◄─► (main    │  │  Ethernet     │  │
│  │ PT1000   │  │  ESPHome)│  │  Controller   │  │
│  │ front-end│  └────┬─────┘  └───────────────┘  │
│  └──────────┘       │                           │
│  ┌──────────┐  ┌────┴─────┐  ┌───────────────┐  │
│  │ MCP6004  │  │ MAX13487 │  │  ADS1115      │  │
│  │ 4ch OPV  │  │  RS-485  │  │  Button ADC   │  │
│  └──────────┘  └──────────┘  └───────────────┘  │
└─────────────────────┬───────────────────────────┘
                      │ 10-pin ribbon (CN3)
┌─────────────────────┴───────────────────────────┐
│              POWER BOARD (Power Board V2)       │
│                                                 │
│  ┌──────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ HLK-PM01 │  │ 2× BTA41     │  │ 2× SRD    │  │
│  │ AC→5V    │  │ TRIAC pumps  │  │ relays    │  │
│  └──────────┘  │ + MOC3021    │  │ valves    │  │
│                │ + snubber    │  └───────────┘  │
│  ┌──────────┐  └──────────────┘                 │
│  │ ZeroCross│  MB10S bridge + LTV-817C          │
│  │ detector │  → ZEROCROSS signal to ESP32      │
│  └──────────┘                                   │
└─────────────────────────────────────────────────┘
```

<div align="center">
    <img width="50%" src="/Images/Complete%20system%20V2.jpg">
</div>

---

## 📐 Hardware Details

### Control Board — Full Board V2.0 (6-page schematic)

<div align="center">
    <img width="40%" src="/Images/3D%20Board%20front%20pinout.png">
    <img width="40%" src="/Images/3D%20Board%20back%20pinout.png">
</div>

#### Page 1 · Microcontroller & Power

- **ESP32-S3** (U1) — main application processor running ESPHome. Controls all outputs, reads sensors, manages Ethernet and RS-485.
- **ATmega32U4** (U31) — dedicated analog co-processor. Drives the PT1000 OPV front-end, sends converted temperature values to the ESP32-S3 via UART (`TEMP-TX` / `TEMP-RX`). Has its own USB-C (USB4) for independent firmware upload and an ICSP header (H9) for UPDI/ISP programming.
- **3.3V power**: BL1117-33CX LDO (U33) fed from the 5V rail. Protected by a WPM2015-3 P-FET ORing between USB-C 5V (USB5V) and board 5V (+5VP), with a 1N5819 Schottky for reverse-polarity protection.
- RESET and BOOT tactile buttons for ESP32-S3; RESET button also present for ATmega32U4.
- Power LED (XL-2012SURC red) on 3.3V via 1kΩ.

#### Page 2 · Ethernet

- **W5500** (U12) — hardware TCP/IP offload chip connected to ESP32-S3 via SPI1 (`SPI1_MOSI/MISO/SCK/CS`). Reset line (`RST_NET`) and interrupt line (`INT_NET`) connected to dedicated ESP32-S3 GPIOs.
- Magnetics: **HR911105A** integrated RJ45 with built-in transformer (1CT:1CT) and LEDs — LINK (yellow) and ACT (green).
- 25MHz crystal (X1) for W5500 clock. Pi-filter (HCB2012KF-121T50 ferrite beads L1/L2) on the 3.3VNET rail for noise isolation.
- Proper termination: 49.9Ω series resistors on TX pairs; 82Ω series on RX pairs; 6.8nF and 10nF AC coupling capacitors; 22nF center-tap capacitor.
- W5500 PMODE pins floating (auto-negotiate 10/100 with auto-MDIX).

#### Page 3 · I/O, PCF8574 & LCD

- **5-button keypad**: UP, DOWN, LEFT, RIGHT, OK (KH-6X6X8H-TJ tactile switches) wired to an **ADS1115** (U3) via a resistive voltage divider (1kΩ + 1MΩ pull-down). The ADS1115 reads button combinations on AIN0; ALERT/RDY can generate interrupts.
- **PCF8574M** (U9) — 8-bit I2C I/O expander that buffers output signals (MOTORE1-3, VALVOLA1-2, ZEROCROSS) to the Power Board via the **SN74HCT14** (U10) Schmitt-trigger hex inverter. Note: this path is **not suitable for PWM signals** (as marked on the schematic).
- **20×4 I2C LCD** (LCD1) connected via a standard PCF8574-based I2C backpack module.
- Two 5-pin I2C expansion headers (H1, H2) provide 3.3V / 5V, SDA, SCL for external I2C peripherals.
- 10-pin IDC connector (CN3) carries all inter-board signals: MOTORE1, MOTORE2, ZEROCROSS, VALVOLA1, VALVOLA2, 3.3V, 5V, +5VP (relay coil), GND.

#### Page 4 · RS-485

- **MAX13487EESA+T** (U5) — half-duplex RS-485 transceiver with integrated auto-direction control (no DE/RE GPIO needed). VCC at 5V, with a `SN74LVC1G125` (U4) level shifter on the RO output to convert from 5V to 3.3V logic for ESP32-S3 RX.
- **120Ω** termination resistor (R21) across A/B lines.
- **4.7kΩ** fail-safe bias resistors (R20, R22) on A and B.
- **SMBJ6.8A** TVS diodes (U13, U14, U15) on A line, B line, and GND for surge/ESD protection on the RS-485 bus.
- Self-resetting polyfuses (F1, F2: nSMD005) on A and B lines.
- Screw terminal connector (CN10: WJ2EDGRC-5.08-3P) for field wiring.

#### Page 5 · ATmega32U4

- **ATmega32U4-MU** (U31) in QFN-44 package. Clocked at 16MHz (crystal X2 with 22pF load caps U29/U30).
- USB (D+/D-) protected by a **DALC208SC6** (D8) ESD array with 22Ω series resistors (R97, R98).
- ICSP/SPI header H9 exposes MOSI, MISO, CLK, RESET for external programmer access.
- Communicates with ESP32-S3 via UART: `PD2(RXD1)` → `TEMP-RX` on ESP32; `PD3(TXD1)` → `TEMP-TX`.
- ADC pins PF0–PF7 connected to the OPV amplifier outputs (TEMP_OUT1–4) and enable signals (TEMP_ENB1–4) controlled by PC7/PC6/PB6/PB5.
- `OPV_ENB` line (PC7 / PE2) allows the ATmega to power-gate the entire OPV section.
- OneWire DS18B20 bus on a separate 3-pin screw terminal (CN5) with 4.7kΩ pull-up (R66).

#### Page 6 · PT1000 Amplifier

- **MCP6004T-I/SL** (U32) — quad rail-to-rail op-amp, all four channels used, one per PT1000 sensor.
- Each channel implements an **inverting amplifier** with gain set by a ±0.1% tolerance 9.1kΩ feedback resistor (R90/R85/R74/R81) and 1kΩ input resistor (R91/R86/R75/R80). This gives a gain of –9.1.
- 1MΩ resistors (R87/R88/R71/R72) provide a DC bias path to the non-inverting input (tied to GND reference).
- Each output is low-pass filtered: 1kΩ (R89/R76/R78/R73) + 1µF (C46/C48/C47/C45) forming a ~160Hz corner frequency to reject switching noise.
- Per-channel enable switches (`TEMP_ENB1–4`) built from 4.7kΩ / 4.7kΩ resistor pairs allow the ATmega to multiplex or disable channels individually.
- **DALC208SC6** (D6) ESD protection array on all four sensor input lines (IN+1 through IN+4) before they reach the OPV inputs.
- OPV supply (VDD/VSS) bypassed with 100nF (C51/C52), power-gated via `OPV_ENB`.
- PT1000 connectors: CN9 / CN8 / CN6 / CN7 (WJ2EDGRC-5.08-3P screw terminals, one per sensor channel).

---

### Power Board — Power Board V2.0 (2-page schematic)

<div align="center">
    <img width="40%" src="/Images/3D%20Board%20Power%20front%20pinout.png">
    <img width="40%" src="/Images/3D%20Board%20Power%20back%20pinout.png">
</div>

#### Page 1 · Power Supply, Connector & Relays

- **AC mains input** enters through a 5×20mm fuse (F2/F3) and a varistor (R4: RM10D561KD24E100) + EMI filter choke (L1: UU9.8Y-10mH) before reaching the **HLK-PM01** (U8) isolated AC-DC module, which outputs 5VDC. A 100nF capacitor (C1) on the AC side suppresses differential-mode noise.
- An additional slow-blow fuse (F4: 5G.300021) provides secondary protection.
- The 5V rail powers relay coils, optocouplers, and feeds through the 10-pin IDC (CN3) to the control board.
- **POWER LED** indicator on the Power Board side.
- AC line (L, N) also distributed to motor output screw terminals (M1, M2) and to TRIAC load outputs.
- **DBT50G-9.5-2P** connectors (U2, U3, U6) for L, N, and motor output wiring.
- Four PCB screw posts (SCREW1–4) for chassis/DIN rail mounting.

**Relay section (Valve 1 & Valve 2):**
- Two identical circuits, each using:
  - **BC547** NPN transistor (Q1, Q3) to drive the relay coil from a 3.3V logic signal via 1kΩ base resistor.
  - **SRD-05VDC-SL-C** 5V SPDT relay (RLY1, RLY2) switched by the transistor.
  - **1N4007** flyback diode (D2, D3) across the relay coil.
  - **PC817C** optocoupler (U11, U14) provides galvanic isolation between logic-side (`VALVOLA1/2` signals, 3.3V-fed via 1kΩ) and the relay driver side (5VUSB referenced). Output resistor 1kΩ.
  - RELE' LED indicators (green XL-2012SURC via 220Ω) for visual relay status.
  - 3-pin screw terminal outputs (CN1, CN2) for valve wiring.

#### Page 2 · TRIAC Firing & Zero-Cross Detection

**TRIAC circuits (Pump 1 & Pump 2) — identical topology:**
- **MOC3021M** (U10, U13) — non-zero-crossing TRIAC optocoupler (zero-cross sync is handled in software using the ZEROCROSS signal). LED drive current set by 150Ω / 100Ω resistors (R24/R25) on the logic side.
- Gate current limiting resistor 360Ω (R9, R15) + snubber network: 47nF (C2/C4) + 100Ω (R11/R17) in series across the TRIAC, plus 100nF (C3/C5) across the main terminals.
- Gate-side 470Ω resistor (R10, R16) to limit peak gate current into the TRIAC.
- **BTA41-600BRG** (Q2, Q4) — 40A/600V TO-220 TRIAC. Rated far above typical domestic pump currents for long-term reliability and inrush tolerance.
- **XSD1226-307** (U12, U15) — TRIAC thermal fuse (307°C) in series with the gate resistor for thermal runaway protection.
- Output screw terminals for motors M1 and M2 (Line + Neutral).

**Zero-cross detection:**
- **MB10S** bridge rectifier (D1) connected directly across Line (LP) and Neutral (NP) through four 47kΩ resistors (R1/R2/R5/R6) as a current limiter (total ~188kΩ into a 230V mains, ≈1.2mA peak).
- **LTV-817-C** (U4) optocoupler with 10kΩ pull-up (R3) on the output side (3.3V). Generates the `ZEROCROSS` signal — a pulse train at twice the mains frequency (100Hz for 50Hz mains) that is routed to the ESP32-S3 for phase-synchronised TRIAC firing.

---

## 🔌 Inter-Board Connector Pinout (CN3 — DC3-2.54-10PAS)

| Pin | Signal | Direction | Description |
|---|---|---|---|
| 1 | MOTORE2 | PB → CB | Pump 2 control signal |
| 2 | MOTORE1 | PB → CB | Pump 1 control signal |
| 3 | ZEROCROSS | PB → CB | Zero-cross detection pulse |
| 4 | 5V | PB → CB | 5V power from HLK-PM01 |
| 5 | 3V3 | CB → PB | 3.3V logic reference |
| 6 | 5VUSB | CB → PB | USB 5V (relay coil supply) |
| 7 | — | — | (reserved/GND) |
| 8 | VALVOLA1 | CB → PB | Valve 1 control |
| 9 | VALVOLA2 | CB → PB | Valve 2 control |
| 10 | GND | — | Common ground |

> CB = Control Board, PB = Power Board

---

## 🌡️ PT1000 Sensing: Recommended Wiring

Each PT1000 connects to a 3-pin screw terminal on the Control Board (CN6–CN9). The sensor is wired in a **2-wire configuration** to the OPV input. For best accuracy, use matched cable runs and consider the per-channel gain/offset calibration available in the ATmega firmware.

| Connector | Sensor Location | Signal |
|---|---|---|
| CN9 | Solar collector (panel) | IN+1 → TEMP_OUT1 |
| CN8 | Primary storage tank | IN+2 → TEMP_OUT2 |
| CN6 | Return / secondary circuit | IN+3 → TEMP_OUT3 |
| CN7 | Secondary tank / boiler outlet | IN+4 → TEMP_OUT4 |

---

## ⚙️ Control Logic Overview

The ESP32-S3 implements the following logic layers in ESPHome:

1. **Temperature acquisition** — ATmega32U4 reads 4× PT1000 channels via the OPV front-end, applies per-channel IIR filtering and gain/offset calibration, and streams the values over UART to the ESP32-S3.
2. **Differential control (ΔT)** — Compares solar collector temperature against tank temperatures. Activates `MOTORE1` (solar pump) when ΔT exceeds a configurable threshold; deactivates with hysteresis.
3. **Dual-tank switching** — Activates `VALVOLA1` / `VALVOLA2` to direct hot fluid to the tank with the lowest temperature, maximising energy storage.
4. **Secondary pump** — `MOTORE2` manages heat exchange with the gas boiler loop based on secondary tank differentials.
5. **Protections**:
   - **Anti-freeze**: Forces pump on when collector temperature drops below a configurable minimum.
   - **Overheating**: Limits pump runtime or diverts flow when tank temperature exceeds maximum.
   - **Sensor fault**: Detects open-circuit (NaN, T < –10°C) or short-circuit (T > 200°C) PT1000 conditions.
6. **TRIAC zero-cross firing** — Pump activation is synchronised to the mains zero-crossing via the `ZEROCROSS` interrupt for reduced EMI and inrush.

<div align="center">
    <img width="40%" src="/Images/Front%20panel.jpg">
</div>

---

## 📁 Repository Structure

```
DiSoCon/
├── 3D Model/                         # 3D renders and model files
├── Hardware/                         # Schematics, PCB Gerbers, EasyEDA sources
│   ├── SCH_Full_Board_V2.0.pdf       # Control board schematic (6 pages)
│   └── SCH_Power_Board_V2.0.pdf      # Power board schematic (2 pages)
├── Images/                           # Board renders, photos, diagrams
├── Software/                         # Firmware source code
│   ├── ESPHome YAML                  # ESPHome configuration for ESP32-S3
│   └── ATmega32U4/                   # Arduino sketch for PT1000 co-processor
├── LICENSE
└── README.md
```

---

## 🚀 Getting Started

### 1. Hardware

- Order PCBs from Gerber files (JLCPCB)
- Assemble the Power Board first; verify 5V output before connecting the Control Board
- Flash the ATmega32U4 via ICSP header H9 (Arduino Nano as ISP or dedicated programmer)
- Flash ESPHome to the ESP32-S3 via USB-C (USB3 connector)

### 2. Firmware

```bash
# Clone the repo
git clone https://github.com/carletz/DiSoCon

# Flash ESPHome (first time via USB)
esphome run Software/main.yaml

# Subsequent updates via OTA (Ethernet or WiFi)
```

### 3. Home Assistant Integration

Add the device to ESPHome dashboard. All sensors, pumps, and valve states are automatically exposed as Home Assistant entities. MQTT topics follow ESPHome conventions.

---

## ⚠️ Safety & Disclaimer

> **This project involves mains voltage (230V AC). Working with mains voltage is potentially lethal.**

This project is provided **for educational and inspiration purposes only**. The author(s) accept **no responsibility** for damage to equipment, persons, or property arising from the use of this design.

Any real-world installation must be performed by a **qualified electrician** in compliance with local electrical codes and safety regulations (e.g. IEC 60364 in Europe).

By using this material you acknowledge it is offered **as-is, without warranty of any kind**.

---

## 📅 Version History

| Version | Date | Notes |
|---|---|---|
| V2.0 | March 2026 | Integrated control + analog measurement on single board; ATmega32U4 co-processor; EasyEDA redesign |
| V1.0 | April 2025 | Separate controller and temperature measurement boards |

---

## 👤 Author

**carletz** — [@carletz on GitHub](https://github.com/carletz)

[YouTube: carletz](https://www.youtube.com/@carletzslug)

[OSHWLab: carletz](https://oshwlab.com/carletz.slug/works)

---

## 📜 License

This work — hardware schematics, PCB designs, firmware, and documentation — is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0)**.

[![CC BY-NC-SA 4.0](https://licensebuttons.net/l/by-nc-sa/4.0/88x31.png)](https://creativecommons.org/licenses/by-nc-sa/4.0/)

**You are free to:**
- **Share** — copy and redistribute the material in any medium or format
- **Adapt** — remix, transform, and build upon the material

**Under the following terms:**
- **Attribution** — You must give appropriate credit, provide a link to the license, and indicate if changes were made.
- **NonCommercial** — You may not use the material for commercial purposes.
- **ShareAlike** — If you remix, transform, or build upon the material, you must distribute your contributions under the same license.

See [`LICENSE`](./LICENSE) for the full license text.
