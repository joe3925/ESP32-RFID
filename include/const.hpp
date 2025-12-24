// Contains constants or pins with roles

#pragma once

#include <cstdint>
#define PN532_SCL (22)
#define PN532_SDA (21)
#define PN532_IRQ (26)
#define PN532_RSC (32)
constexpr uint8_t LCD_D7 = 13;
constexpr uint8_t LCD_D6 = 14;
constexpr uint8_t LCD_D5 = 27;
constexpr uint8_t LCD_D4 = 19;
constexpr uint8_t LCD_RW = 23;
constexpr uint8_t LCD_RS = 18;
constexpr uint8_t LCD_E = 25;

constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;

constexpr uint8_t FPGA_WAKE = 33;

constexpr int FPGA_RX = 16;
constexpr int FPGA_TX = 17;

constexpr uint8_t FRAME_MAGIC = 0xA5;
constexpr uint8_t FPGA_READY = 0x52;

constexpr uint8_t RES_ALLOW = 0x01;
constexpr uint8_t RES_DENY = 0x00;
constexpr uint8_t RES_OK = 0x02;

constexpr uint32_t READY_TIMEOUT_MS = 1000;
constexpr uint32_t RESP_TIMEOUT_MS = 1000;

constexpr char WIFI_SSID[] = "Sunny 2.4Ghz";
constexpr char WIFI_PASS[] = "8943667abc";