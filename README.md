# STM32 Multi-Sensor Data Logger with IoT Connectivity

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
[![STM32](https://img.shields.io/badge/STM32-F103C8T6-blue)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
[![CubeIDE](https://img.shields.io/badge/IDE-STM32CubeIDE-darkblue)](http://st.com/en/development-tools/stm32cubeide.html)
[![FatFS](https://img.shields.io/badge/FatFS-R0.15-darkgreen)](http://elm-chan.org/fsw/ff/00index_e.html)
[![ESP8266](https://img.shields.io/badge/NodeMCU-ESP8266-orange)](https://www.espressif.com/en/products/socs/esp8266)
[![MQTT](https://img.shields.io/badge/MQTT-3.1.1-green)](https://mqtt.org/)

## Table of Contents
- [Project Overview](#project-overview)
- [Video Demonstrations](#video-demonstrations)
- [Hardware](#hardware-components)
- [Task Scheduling](#task-scheduling)
- [Drivers](#mqtt-driver)
- [Getting Started](#getting-started)
- [Resources](#related-projects)
- [Contact](#contact)

## Project Overview

The **STM32 Multi-Sensor Data Logger with IoT Connectivity** is a comprehensive embedded system that reads data from multiple sensors, displays real-time information on a 16x2 LCD, logs data to a MicroSD card, and publishes readings to the cloud via Wi-Fi. The system demonstrates professional embedded concepts including sensor data acquisition, data storage, real-time control, and task scheduling without using an RTOS.

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

## Hardware Components

| Component | Quantity | Description |
|-----------|----------|-------------|
| **STM32F103C8T6** | 1 | "Blue Pill" development board with 72MHz Cortex-M3 |
| **NodeMCU ESP8266** | 1 | WiFi module running AT firmware (acts as modem) |
| **DHT11** | 1 | Temperature & Humidity sensor (1-Wire protocol) | 
| **MPU6050** | 1 | 6-axis inertial measurement unit (accelerometer + gyroscope + temperature) |
| **DS3231 RTC Module** | 1 | Precision RTC with built-in temperature sensor (±3°C accuracy) |
| **LCD 16x2 with I2C** | 1 | Character display module with I2C backpack (PCF8574) |
| **MicroSD Card Adapter** | 1 | SPI interface module for SD card |
| **MicroSD Card** | 1 | Storage media (FAT32 formatted) |
| **Push Buttons** | 3 | Two-leg tactile switches for user input |
| **USB-to-Serial Converter** | 1 | CP2102 / CH340 / FTDI for UART |
| **ST-Link V2 Programmer** | 1 | For flashing/debugging STM32 |

### Pin Configuration

| Peripheral | Pin | Connection | Notes |
|------------|-----|------------|-------|
| **DHT11** | PB0 | DATA | Built-in 1kΩ pull-up |
| | 5V | VCC | Power |
| | GND | GND | Common ground |
| **MPU6050** | PB10 | SCL | I2C2 clock (shared with LCD & DS3231) |
| | PB11 | SDA | I2C2 data (shared with LCD & DS3231) |
| | 5V | VCC | Power (module includes voltage regulator) |
| | GND | GND | Common ground |
| **DS3231 RTC** | PB10 | SCL | I2C2 clock (shared with MPU6050 & LCD) |
| | PB11 | SDA | I2C2 data (shared with MPU6050 & LCD) |
| | 5V | VCC | Power (module includes voltage regulator) |
| | GND | GND | Common ground |
| **LCD 16x2 I2C** | PB10 | SCL | I2C2 clock (shared with MPU6050 & DS3231) |
| | PB11 | SDA | I2C2 data (shared with MPU6050 & DS3231) |
| | 5V | VCC | Power (LCD backlight needs 5V) |
| | GND | GND | Common ground |
| **MicroSD Card Adapter** | PB3 | SCK | SPI1 Clock |
| | PB4 | MISO | SPI1 Master In Slave Out |
| | PB5 | MOSI | SPI1 Master Out Slave In |
| | PB6 | CS | Chip Select |
| | 3.3V | VCC | Power |
| | GND | GND | Common ground |
| **ESP8266 NodeMCU** | PA9 | TX to ESP RX | USART1 for AT commands |
| | PA10 | RX to ESP TX | USART1 for AT commands |
| | 3.3V | VCC | Power |
| | GND | GND | Common ground |
| **USART2** | PA2 | TX to USB-Serial RX | 115200 baud, 8-N-1 |
| | PA3 | RX to USB-Serial TX | Optional for commands |
| **Button 1** | PA0 | Mode select | Input with internal pull-up |
| **Button 2** | PA1 | Save Data | Input with internal pull-up |
| **Button 3** | PA7 | Read Data | Input with internal pull-up |

> **Note:** MicroSD cards require 3.3V logic, but most adapter modules include onboard voltage regulators and level shifters, allowing them to be powered with 5V.

The LCD display, MPU6050, and DS3231 RTC share the same I2C bus (PB10/SCL, PB11/SDA) with different addresses:

| Device | I2C Address (7-bit) | 8-bit Write | 8-bit Read |
|--------|---------------------|-------------|------------|
| **DS3231 RTC** | 0x68 | 0xD0 | 0xD1 |
| **MPU6050** | 0x69 | 0xD2 | 0xD3 |
| **LCD (PCF8574)** | 0x27 | 0x4E | 0x4F |

🔗 [View Custom Written I2C Driver Source Code](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/i2c2.c) <br>
> **Note**: All peripherals are used as pre-built modules. The LCD module uses a PCF8574 I2C backpack. 

## Task Scheduling

The system uses a **10ms timer-based control loop** with independent counters for each task. TIMER3 is configured to drive the main control loop.

### Task Frequencies

| Task | Frequency | Period | Execution |
|------|-----------|--------|-----------|
| **DHT11 Read** | 1 Hz | 1 second | Every 100 loops |
| **MPU6050 Read** | 20 Hz | 50 ms | Every 5 loops |
| **LCD Update** | 10 Hz | 100 ms | Every 10 loops |
| **SD Data Logger Task** | 0.2 Hz | 5 seconds | Every 500 loops (uncomment in main loop)|
| **Button Status Check** | 100 Hz | 10 ms | Every loop |
| **Button Interrupts** | Event-driven | On press | EXTIx + TIM4 debounce |
| **MQTT DHT11 Publish** | 0.0667 | 15 seconds | every 1500 loops |
| **MQTT MPU6050 Publish** | 0.1 | 10 seconds | every 1000 loops |

### Timer Configuration

| Timer | Resolution | Purpose |
|-------|------------|---------|
| **DWT** | 1µs | DHT11 1-Wire protocol precise timing |
| **TIMER2** | 1ms | System millisecond counter and delays |
| **TIMER3** | 0.1ms | **10ms control loop scheduler** (heartbeat) |
| **TIMER4** | 0.1ms | Button debouncing (50ms) |

🔗 [View DWT Driver (Microsecond Delay)](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/dwt.c)  
🔗 [View TIMER2 Driver (Millisecond Counter)](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/timer2.c)  
🔗 [View TIMER3 Driver (10ms Heartbeat)](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/timer3.c)  
🔗 [View Button & TIMER4 Driver (Debounce)](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/button.c)

> **Note:** DWT (Data Watchpoint and Trace) is a built-in peripheral in ARM Cortex-M3 cores that provides a cycle counter running at CPU frequency (72MHz). This gives ~13.9ns resolution, making it ideal for generating precise microsecond delays required by the DHT11 1-Wire protocol. Unlike traditional timer-based delays, DWT does not occupy a dedicated timer peripheral and continues running in the background.

## MQTT Driver

The STM32 implements the **MQTT 3.1.1 protocol** from scratch, handling packet construction and parsing without external libraries. All packets are built manually using custom buffer manipulation functions.

### How MQTT Works

MQTT is a **binary protocol** that uses lightweight fixed and variable headers. Each packet starts with a control byte followed by remaining length:
- Byte 0: Packet Type + Flags (1 byte) 
- Byte 1+: Remaining Length (1-4 bytes)
- Variable Header + Payload (optional)

| Packet | Binary | Purpose |
|--------|--------|---------|
| **CONNECT** | `0x10` | Establishes connection with broker |
| **CONNACK** | `0x20` | Broker acknowledges connection |
| **PUBLISH** | `0x30` | Sends data to a topic |
| **SUBSCRIBE** | `0x82` | Requests messages from a topic |
| **PINGREQ** | `0xC0` | Keeps connection alive |

### ThingSpeak MQTT Broker

ThingSpeak provides a **dedicated MQTT broker** for IoT data logging with:
- Free account with 4 million messages per year
- Built-in data visualization and charts
- Automatic timestamping and data storage
- REST API and MQTT support

> **Important:** The ESP8266 AT firmware does **NOT** support SSL/TLS on port 8883 (MQTT over TLS). The AT command set only supports plain TCP connections on port 1883 or WebSocket on port 80. Therefore, this implementation uses **unencrypted MQTT on port 1883**.

### Complete Stack
```
Application Layer (tasks.c) - Read sensors, format payload, schedule publishes
↓
MQTT Protocol (esp8266.c) - CONNECT, PUBLISH packet builder
↓
ESP8266 AT Commands (esp8266.c) - TCP connection, data transmission (port 1883)
↓
Hardware (STM32 USART1 + ESP8266)
```

### ThingSpeak MQTT Configuration

```c
// ThingSpeak MQTT Broker Configuration (Non-SSL, port 1883)
#define MQTT_BROKER     "mqtt3.thingspeak.com"
#define MQTT_PORT       1883                    // Plain TCP (ESP8266 AT doesn't support 8883)
#define MQTT_CLIENT_ID  "YOUR_CLIENT_ID"
#define MQTT_USERNAME   "YOUR_USERNAME"
#define MQTT_PASSWORD   "YOUR_PASSWORD"
#define MQTT_KEEPALIVE  60

// ThingSpeak Channels
#define CHANNEL_DHT11    "3309559"      // DHT11 Temperature & Humidity
#define CHANNEL_MPU6050  "3309560"      // MPU6050 Accel & Gyro

// ThingSpeak MQTT Topics (API format)
// For publishing: channels/CHANNEL_ID/publish
#define TOPIC_DHT11     "channels/3309559/publish"
#define TOPIC_MPU6050   "channels/3309560/publish"
```
> Note: Publishing every 10-15 seconds automatically keeps the connection alive, eliminating the need for explicit PINGREQ packets. ThingSpeak also provides automatic data storage and visualization.

🔗 [View ESP8266 Driver and MQTT Implementation](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/esp8266.c)  

## MicroSD Card & FatFS Driver

The SD card interface handles all communication between the STM32 and the MicroSD card, from low-level SPI commands to high-level file operations.

### Complete Stack
```
Application Layer (sd_data_logger.c) - CSV formatting, timestamping, button handling
↓
High-Level File API (sd_functions.c) - Mount, read, write, list files
↓
FatFS Middleware (CubeMX generated) - File system implementation
↓
Disk I/O Interface (sd_diskio.c) - FatFS hardware abstraction
↓
Low-Level SPI Driver (sd_spi.c) - Raw SPI commands, DMA, timing
↓
Hardware (STM32 SPI1 + SD Card Adapter)
```
#### 1. **Low-Level Driver (`sd_spi.c`)**
- Full SD protocol implementation (CMD0, CMD8, ACMD41, CMD17, CMD24, etc.)
- SDHC/SDSC detection and handling
- Single and multi-block read/write with DMA support
- CRC and error checking

#### 2. **FatFS Hardware Interface (`sd_diskio.c`)**
- Standard FatFS disk I/O functions (initialize, read, write, ioctl)
- Connects hardware driver to FatFS middleware
- Parameter checking and status reporting

#### 3. **High-Level File API (`sd_functions.c`)**
- **Mount/Unmount** - Filesystem detection and mounting
- **Auto-format** - Detects unformatted cards and formats them (512/4096-byte fallback)
- **File Operations** - Create, write, append, read, delete, rename
- **Directory Operations** - Create folders, recursive listing with sizes
- **Space Information** - Get free and total space in KB
- **CSV Parsing** - Read and parse CSV files

### SD Card Data Format

Data is saved to `sensor_data.csv` with the following format:

```csv
DateTime,DHT11_C,DHT11_%,AX_g,AY_g,AZ_g,GX_dps,GY_dps,GZ_dps
2026-03-26 14:30:45,25.5,60.2,0.012,0.005,9.810,1.23,2.34,0.56
2026-03-26 14:31:00,25.6,60.3,0.011,0.006,9.812,1.25,2.36,0.58
```

🔗 [View sd_spi.c - Low-Level Driver](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/sd_spi.c)<br>
🔗 [View sd_diskio.c - FatFS Interface](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/sd_diskio.c) <br>
🔗 [View sd_functions.c - High-Level API](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/sd_functions.c) <br>
🔗 [View sd_data_logger.c - Application Layer driver](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/sd_data_logger.c)

> Note: The SD card uses SPI1 with pins PB3 (SCK), PB4 (MISO), PB5 (MOSI), and PB6 (CS). The FatFS middleware is configured for 512-byte sectors and supports both SDSC and SDHC cards.

## MPU6050 IMU Driver

The MPU6050 is an 6-axis inertial measurement unit (IMU) with an **I2C interface**.

The driver provides two ways to read sensor data:
1. **Raw Data**: Direct 16-bit integer values from registers (±32768 range)
2. **Scaled Data**: Converted to physical units using sensitivity factors

**Implementation:**
- **Burst read** of all 14 data bytes in a single I2C transaction
- Data stored in global structure for access by other tasks
- **Data Flow:** I2C Read (0x3B-0x48) → 14 bytes → Store raw values → Scale for display

### Output Values

| Measurement | Raw Range | Scale Factor | Physical Unit |
|-------------|-----------|--------------|---------------|
| **Accelerometer** | ±32768 | ±2g / ±4g / ±8g / ±16g | g (gravities) |
| **Gyroscope** | ±32768 | ±250 / ±500 / ±1000 / ±2000 | °/s (dps) |
| **Temperature** | ±32768 | 340.0 per °C + 36.53 offset | °C |

🔗 [View MPU6050 Driver Source Code](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/mpu6050.c)

> **Note**: LCD display and SD Card logging use **scaled values** in physical units. Raw 16-bit values are stored in seperate structure and could be extracted if necessary. 

## DHT11 Sensor Driver

The DHT11 uses a **single-wire protocol** with precise timing:

| Phase | Duration | Description |
|-------|----------|-------------|
| **Start Signal** | 18ms LOW + 20µs HIGH | MCU wakes sensor |
| **Sensor Response** | 80µs LOW + 80µs HIGH | Sensor acknowledges |
| **Bit "0"** | 50µs LOW + 26-28µs HIGH | Logic 0 |
| **Bit "1"** | 50µs LOW + 70µs HIGH | Logic 1 |
| **Data Frame** | 40 bits | 5 bytes (humidity ×2 + temp ×2 + checksum) |

Instead of measuring pulse width, I used a **simpler approach** looking at datasheet:

For each bit:
1. Wait for line to go HIGH
2. Delay exactly 40µs
3. If line still HIGH → logic 1 <br>
   If line is LOW → logic 0

To ensure the timing is not interrupted, **interrupts are disabled** while communicating with the sensor. The checksum provided by the sensor is used to verify data integrity.

🔗 [View DHT11 Driver Source Code](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/dht11.c)

## DS3231 RTC Driver

The DS3231 is a **precision Real-Time Clock (RTC)** with an integrated temperature-compensated crystal oscillator and **I2C interface**.
The driver provides two ways to access RTC data:
1. **Time/Date Data**: Current time (hours, minutes, seconds) and date (day, month, year)
2. **Temperature Data**: Built-in temperature sensor reading (±3°C accuracy)

**Implementation:**
- **Burst read** of all 7 time registers in a single I2C transaction
- **BCD conversion** handles register format automatically
- **Data Flow:** I2C Read (0x00-0x06) → 7 bytes → Convert BCD → Update time structure
- **Oscillator monitoring** detects power failures via status register

### Output Values

| Measurement | Range | Resolution | Format |
|-------------|-------|------------|--------|
| **Time** | 00:00:00 - 23:59:59 | 1 second | HH:MM:SS |
| **Date** | 01/01/00 - 31/12/99 | 1 day | DD/MM/YY |

🔗 [View DS3231 Driver Source Code](https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger/blob/main/Core/Src/ds3231.c)

> **Note**: LCD display shows **formatted time/date strings** and temperature in physical units. Raw BCD values are automatically converted using internal helper functions. The RTC maintains accurate time even when main power is off using a CR2032 backup battery.

## Getting Started

### Software Prerequisites

| Software | Version | Purpose |
|----------|---------|---------|
| STM32CubeIDE | v1.13.0+ | IDE for development and flashing |
| Serial Terminal | Any (PuTTY, Arduino IDE, Hterm) | For debugging via USART2 |
| ESP8266 AT Firmware | Official or AI-Thinker | Must be flashed to NodeMCU first |

### Setting Up the ESP8266

Before connecting to STM32, ensure the NodeMCU has AT firmware:

1. **Test with PC** using USB and serial terminal:
    ```
    AT 
    OK

    AT+CWMODE=1
    OK

    AT+GMR
    AT version:...
    ```

2. If the NodeMCU has Lua firmware, you'll need to **flash AT firmware first**. The AI-Thinker firmware works well with NodeMCU boards.

### Installation

1. Clone the repository
```bash
git clone https://github.com/rubin-khadka/STM32_MicroSD_Cloud_Logger.git
```
2. Open this project in STM32CubeIDE:
   - `File` → `Open Projects from File System...`
   - Select the cloned directory
   - Click `Finish`

3. Update Configuration
    - Open `main.c` and update your WiFi credentials:
    ```c
    ESP_ConnectWiFi("your_ssid", "your_password", ip_buf, sizeof(ip_buf))
    ```

4. Build & Flash
    - Build: `Ctrl+B`
    - Debug: `F11`
    - Run: `F8` (Resume)

## Related Projects 
- [STM32_MultiSensor_MicroSD_Datalogger](https://github.com/rubin-khadka/STM32_MultiSensor_MicroSD_Datalogger)
- [STM32_ESP8266_DHT11_Thingspeak](https://github.com/rubin-khadka/STM32_ESP8266_DHT11_Thingspeak)
- [STM32_ESP8266_IP_ATCommand](https://github.com/rubin-khadka/STM32_ESP8266_IP_ATCommand)
- [STM32_DHT11_MPU6050_LCD](https://github.com/rubin-khadka/STM32_DHT11_MPU6050_LCD) 

## Resources
- [STM32F103 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
- [STM32F103 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [DHT11 Sensor Datasheet](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf)
- [MPU6050 Sensor Datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
- [RTC DS3231 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf)
- [PCF8574 I2C Backpack Datasheet](https://www.ti.com/lit/ds/symlink/pcf8574.pdf)
- [ESP8266 AT Command Set](https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Command_Set/index.html)
- [NodeMCU Documentation](https://nodemcu.readthedocs.io/en/release/)
- [Flashing AT Firmware to NodeMCU](https://docs.ai-thinker.com/en/esp8266/)

## Project Status
- **Status**: Complete
- **Version**: v1.0
- **Last Updated**: March 2026

## Contact
**Rubin Khadka Chhetri**  
📧 rubinkhadka84@gmail.com <br>
🐙 GitHub: https://github.com/rubin-khadka