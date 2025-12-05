# DiSoCon
# ☀️ Differential Solar Controller – 2025

# UPDATED TO V2... this is the old design

This project is a modular **differential solar thermal controller** designed for systems with multiple thermal storage tanks. It supports:
- A **primary pump** for the solar panels
- A **3-way valve** to switch between **two storage tanks**
- A **secondary pump** for an additional tank, in my case the one heated by a normal gas boiler
- A **final 3-way valve** to manage recirculation between solar tank and normal hot water

The controller is designed to operate autonomously based on temperature differentials and includes logic protection features like anti-freeze and overheat modes.

---
## 🕰️ Project History

I have a solar panel system that generates hot water at home in the warm months. 
The current controller is a classic type without any type of integration with home automation control systems. The system works great but it was time for me to make it smart...

I decided to take inspiration from the manual of my controller, from a triac control of 230V motors, and from a PT1000 probe reader, like the one I currently have installed on the panels and cannot remove or replace. 
For this reason this project was born and now, in the final stages of testing, I would like to share it for inspiration or comments.

---
## 🧩 System Architecture

### 1. Temperature Module (V1.1)
- Interfaces with up to 4 PT1000 temperature sensors and DS18B20 probes for others temperature. The PT1000 are designed for the solar panels, tank and normal hot water
- Provides real-time thermal data for control logic

### 2. Control Logic (V3.2)
- Based on a microcontroller (ESP32-S3)
- Controls triacs for pumps and relays valves (check your voltage and characteristic based on your system, this one is designed for a normal 230V power supply)
- Accepts analog/digital inputs (buttons, override switches, etc.)
- Manages logic for differential temperature thresholds, hysteresis, timing, and safety

### 3. Power Board (V2.0)
- Drives relays and triacs for high-current devices
- Features optoisolated inputs and snubber circuits

---

## 📐 Schematics

All hardware schematics are provided in PDF format:

### 1. Temperature Module (V1.1)
The module is designed to:
- Read temperatures from solar collectors, buffer tanks, and return lines
- Convert sensor values to analog signals via operational amplifiers
- Transmit readings to the control unit via **RS485 serial communication**

It is fully compatible with microcontroller-based systems like ESP32, Arduino, or ATmega32U4 (as used in this design).

- Supports up to **4 analog temperature channels**
- Sensor power enable lines (`TEMP_ENBx`) for selective powering
- Signal amplification and filtering per channel
- Differential RS485 interface for reliable long-distance communication
- Galvanic isolation and protection circuits on all inputs
- USB Type-C interface for programming or debugging

#### 🧠 Functional Overview

1. **Sensor Input**
   - Each temperature channel uses a voltage divider with precise resistors and capacitors for noise filtering.
   - Inputs labeled `IN+1`, `IN+2`, etc., receive analog sensor signals.

2. **Amplification**
   - Signals are routed through MCP6004 op-amps.
   - Outputs `TEMP_OUT1`, `TEMP_OUT2`, etc., carry conditioned analog signals.

3. **Data Handling**
   - The ATmega32U4 samples each analog signal and prepares the data packet.
   - A protocol handler then sends the data via RS485 using the MAX13487 driver.

4. **Communication**
   - Pins `485TX` and `485RX` connect to the control board over twisted pair cabling.
   - The board can be addressed or identified via firmware if needed.

### 2. Control Logic (V3.2) 
This board is the **core controller** of a modular solar thermal system. It processes temperature readings (received via RS485), applies control logic, and operates pumps and 3-way valves for dynamic thermal management across multiple storage tanks.

#### 🎯 Purpose

The main board is designed to:
- Receive temperature data via **RS485** from external sensor boards
- Execute differential logic (ΔT) to optimize energy transfer
- Drive up to **3 pumps** and **2 3-way valves**
- Provide optional control over final delivery circuits
- Connect to network systems via **Ethernet (W5500)** or USB
- Display system status on an **I2C LCD**

#### 🧩 Key Modules & Features

##### 🔧 Microcontroller
- **ESP32-S3** module for logic processing
- USB Type-C for programming and debugging
- I2C ADC (ADS1115) for buttons inputs

##### ⚡ Power & Outputs
- 5 powered outputs for **pumps and valves** (via relay/Triacs)
- "ZEROCROSS" signal input for phase control
- Output enable pins with snubber & protection circuits

##### 🔌 Communication
- RS485 transceiver (**MAX13487**) for remote sensor integration
- W5500 Ethernet controller with integrated magnetics
- USB-C for firmware upload

##### 🖥️ User Interface
- **20x4 I2C LCD** with backlight control
- 5-button interface (UP, DOWN, LEFT, RIGHT, OK)
- LED indicators for relay status

### 3. Power Board (V2.0)
This board provides the **power interface** for a differential solar thermal controller. It manages relays, TRIACs, and optoisolators to drive **AC pumps**, **3-way valves**, and other inductive loads safely and efficiently.

#### 🎯 Purpose

The power board is designed to:
- Switch high-current **AC loads** (pumps) via **TRIACs**
- Drive **DC relays** for directional valve control
- Provide **galvanic isolation** via optocouplers
- Detect **zero-crossing** for synchronized TRIAC firing
- Convert **AC to DC power** with onboard switching regulator for all boards

#### 🧩 Key Modules & Features

##### 🔌 Power Supply
- **HLK-PM01** module for converting AC mains to 5VDC
- EMI filter (inductor + varistor) for surge suppression
- Fuse protection on AC input

##### 🔁 Output Control
- **2 x Relay outputs** for 3-way valve actuation (VALVOLA1 / VALVOLA2)
- **2 x TRIAC circuits** (with MOC3021 + BTA41) for motor/pump control
- Optical isolation on both relays and TRIAC gates

##### 📈 Zero-Cross Detection
- Optocoupler + RC filtering network to detect AC phase crossing
- Signal `ZEROCROSS` sent to the logic board for phase-synced control

---

## ⚙️ Features

- Modular input/output management
- Support for dual-tank switching and secondary heating loops
- Temperature-based control logic with configurable thresholds
- Integration-ready with MQTT, Openhab, and standalone LCD display
- Configurable via analog buttons or MQTT
- Anti-freeze and overheating protections included

---


## 📅 Version Info

- Temperature board: **V1.1**
- Control board: **V3.2**
- Power board: **V2.0**
- Date: April 2025

---

## ⚠️ Disclaimer

This project is provided **for educational purposes only**.

The author(s) of this repository do **not take any responsibility** for:
- Incorrect use of the design or schematics
- Damage to equipment, persons, or property
- Installation in unsafe or non-compliant environments

Any implementation of this project in real-world systems should be done **under the responsibility of a qualified professional**, and in compliance with local regulations and safety standards.

By using this material, you acknowledge that it is offered **as-is, without warranty of any kind**, and you agree to use it **at your own risk**.
