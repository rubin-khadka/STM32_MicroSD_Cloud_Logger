# STM32 Multi-Sensor Data Logger with IoT Connectivity

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
[![STM32](https://img.shields.io/badge/STM32-F103C8T6-blue)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
[![CubeIDE](https://img.shields.io/badge/IDE-STM32CubeIDE-darkblue)](http://st.com/en/development-tools/stm32cubeide.html)
[![FatFS](https://img.shields.io/badge/FatFS-R0.15-darkgreen)](http://elm-chan.org/fsw/ff/00index_e.html)
[![ESP8266](https://img.shields.io/badge/NodeMCU-ESP8266-orange)](https://www.espressif.com/en/products/socs/esp8266)
[![MQTT](https://img.shields.io/badge/MQTT-3.1.1-green)](https://mqtt.org/)

## Project Overview

The **STM32 Multi-Sensor Data Logger** is a comprehensive embedded system that reads data from multiple sensors, displays real-time information on a 16x2 LCD, logs data to a MicroSD card, and publishes readings to the cloud via Wi-Fi. The system demonstrates professional embedded concepts including sensor data acquisition, data storage, real-time control, and task scheduling without using an RTOS.

### Sensor Suite

| Sensor | Interface | Data Acquired |
|--------|-----------|---------------|
| **DHT11** | 1-Wire Protocol | Temperature (°C) & Humidity (%) |
| **MPU6050** | I2C | 3-axis Accelerometer (g), 3-axis Gyroscope (dps) |
| **DS3231** | I2C | Real-Time Clock (Date & Time) |

All sensor data is saved to a MicroSD card in CSV format using the **FatFS file system**.

### User Interaction

Three ways to interact with the system:

1. **16x2 LCD Display** - Primary visual interface showing real-time data in 4 selectable modes:
   - Temperature & Humidity
   - Accelerometer (X, Y, Z in g)
   - Gyroscope (X, Y, Z in dps)
   - Date & Time

2. **Three Push Buttons** (Hardware-debounced with 1s cooldown):
   - **Button 1** - Switch display modes
   - **Button 2** - Save current readings to SD Card
   - **Button 3** - Retrieve all stored data via UART

3. **UART Interface** - Export data in formatted table (115200 baud)

### Cloud Connectivity

- **ESP8266** connects to Wi-Fi
- **MQTT** publishes data to ThingSpeak:
  - Channel 3309559: DHT11 data (15s interval)
  - Channel 3309560: MPU6050 data (10s interval)
- Real-time dashboards for remote monitoring

## Video Demonstrations

### Button 1 - Display Mode Cycling and Hardware Init

https://github.com/user-attachments/assets/14dabe64-e896-4260-8416-0986eb822df1

Pressing Button 1 cycles through four LCD display modes: DHT11 Temp/Hum → Accelerometer → Gyroscope → Date and Time

### Button 2 & 3 - Save and Retrieve Data

https://github.com/user-attachments/assets/e409e185-75fa-4af3-9bf9-d0ec63bac33c

Button 2 saves sensor data to MicroSD card. Button 3 retrieves all stored data and sends via UART.<br>
Both buttons have a 1-second cooldown to prevent accidental multiple presses and allow time for data to be saved.

### Cloud Dashboard & Debug Output

https://github.com/user-attachments/assets/71cbf783-8c4f-4e7b-9c29-2fdef37ea39f

- **Left:** Data received by ThingSpeak showing graph and indicator
- **Right:** UART Output showing device connecting to wifi and showing confirmation that data is sent to ThingSpeak and button press outputs

## Project Schematic

![schematic diagram](https://github.com/user-attachments/assets/8e1bf817-eebf-4f7d-8509-530f936b7406)

