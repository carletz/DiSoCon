# DiSoCon
# ☀️ Differential Solar Controller – 2025

This project is a modular **differential solar thermal controller** designed for systems with multiple thermal storage tanks. It supports:
- A **primary pump** for the solar panels
- A **3-way valve** to switch between **two storage tanks**
- A **secondary pump** for an additional tank, in my case the one heated by a normal gas boiler
- A **final 3-way valve** to manage recirculation between solar tank and normal hot water

The controller is designed to operate autonomously based on temperature differentials and includes logic protection features like anti-freeze and overheat modes.

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

- SCH_TEMPERATURE V1.1_2025-04-24.pdf
- SCH_CENTRALINA CONTROLLO V3.2_2025-04-24.pdf
- SCH_CENTRALINA POTENZA V2.0_2025-04-24.pdf

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
