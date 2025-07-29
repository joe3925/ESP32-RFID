#pragma once

#include <cstdint>
constexpr uint8_t RFID_SS = 5;
constexpr uint8_t RFID_SCK = 18;
constexpr uint8_t RFID_MOSI = 23;
constexpr uint8_t RFID_MISO = 19;
constexpr uint8_t RFID_RST = 21;
constexpr uint8_t RFID_IRQ = 22;

// Only one dio is needed for the led. High is auth
constexpr uint8_t LED = 32;

constexpr uint8_t FPGA_WAKE = 13;

constexpr int FPGA_RX = 16; 
constexpr int FPGA_TX = 17; 

constexpr uint8_t FRAME_MAGIC   = 0xA5;
constexpr uint8_t FPGA_READY    = 0x52;

constexpr uint8_t RES_ALLOW     = 0x01;
constexpr uint8_t RES_DENY      = 0x00;
constexpr uint8_t RES_OK        = 0x02; 

constexpr uint32_t READY_TIMEOUT_MS = 20;
constexpr uint32_t RESP_TIMEOUT_MS  = 50;

constexpr char WIFI_SSID[] = "Area51";
constexpr char WIFI_PASS[] = "GreenTD70!#%";