# ESP32-RFID

An ESP32-based RFID access control system using the PN532 NFC reader with LCD display output and FPGA integration.

## Features

- **RFID/NFC Card Reading**: Uses Adafruit PN532 reader (I2C interface)
- **LCD Display**: 16x2 character LCD for status feedback
- **WiFi Connectivity**: Network support for cloud integration
- **FPGA Communication**: Serial communication with external FPGA for access control logic
- **Firebase Support**: Ready for Firebase Realtime Database integration (currently commented out)

## Hardware Requirements

- ESP32 Development Board
- Adafruit PN532 NFC/RFID Reader Module
- 16x2 LCD Display (HD44780 compatible)
- External FPGA (for access control validation)
- Connecting wires and breadboard

## Software Requirements

### Toolchain

- **[PlatformIO](https://platformio.org/)** - Primary build system
- **PlatformIO Core (CLI)** or **PlatformIO IDE** (VS Code extension)

### Dependencies

The following libraries are automatically installed via PlatformIO: 

- `espressif32` platform
- `arduino` framework
- `Arduino_MFRC522v2` (v2.0.1)
- `Firebase Arduino Client Library for ESP8266 and ESP32` (v4.4.17)
- `LiquidCrystal` (v1.0.7)
- `Adafruit PN532` (v1.3.4)

## Build Instructions

### 1. Install PlatformIO

**Via VS Code:**
```bash
# Install VS Code, then install the PlatformIO IDE extension
```

**Via CLI:**
```bash
pip install platformio
```

### 2. Clone the Repository

```bash
git clone https://github.com/joe3925/ESP32-RFID.git
cd ESP32-RFID
```

## 3. Configuration

Create an `include/env.hpp` file with your WiFi and Firebase credentials (if using Firebase):

```cpp
#ifndef ENV_HPP
#define ENV_HPP

#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_password"

// Firebase (optional)
#define API_KEY_REAL "your_firebase_api_key"
#define DATABASE_URL_REAL "your_database_url"
#define EMAIL "your_email"
#define PASSWORD "your_password"

#endif
```

## 4. Pin Configuration

Refer to `include/const.hpp` for pin definitions. Key connections:

- **PN532 NFC**:  I2C (SDA/SCL) + IRQ pin
- **LCD**:  Standard 4-bit mode connections
- **FPGA**: Serial2 (RX/TX)

## 5. Flash 

General --> Build --> Upload and Monitor --> Check Serial Monitor For Inputs

## Usage

1. Power on the ESP32
2. Wait for "Ready" message on LCD
3. Present an RFID/NFC card to the PN532 reader
4. System validates card via FPGA and displays "Approved!" or "Not auth"

## Project Structure

```
ESP32-RFID/
├── include/          # Header files
├── src/              # Source files
│   ├── main. cpp      # Main application logic
│   ├── fpga.cpp      # FPGA communication
│   ├── wifi.cpp      # WiFi management
│   └── util.cpp      # Utility functions
├── platformio.ini    # PlatformIO configuration
└── README.md         # This file
```

## Author

[joe3925](https://github.com/joe3925)
[pasinponsaw](https://github.com/pasinponsww)