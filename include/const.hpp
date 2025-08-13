// Contains constants or pins with roles

#pragma once

#include <cstdint>
#define PN532_SCL (22)
#define PN532_SDA (21) 
#define PN532_IRQ (16)
#define PN532_RSC (32)
constexpr uint8_t LCD_D7 = 13;
constexpr uint8_t LCD_D6 = 14;
constexpr uint8_t LCD_D5 = 27;
constexpr uint8_t LCD_D4 = 19;
constexpr uint8_t LCD_RW = 23;
constexpr uint8_t LCD_RS = 17;
constexpr uint8_t LCD_E = 25;
constexpr uint8_t LCD_VO = 26;

constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;

// Only one dio is needed for the led. High is auth
constexpr uint8_t LED = 32;

constexpr uint8_t FPGA_WAKE = 13;

constexpr int FPGA_RX = 1;
constexpr int FPGA_TX = 3;

constexpr uint8_t FRAME_MAGIC = 0xA5;
constexpr uint8_t FPGA_READY = 0x52;

constexpr uint8_t RES_ALLOW = 0x01;
constexpr uint8_t RES_DENY = 0x00;
constexpr uint8_t RES_OK = 0x02;

constexpr uint32_t READY_TIMEOUT_MS = 20;
constexpr uint32_t RESP_TIMEOUT_MS = 50;

constexpr char WIFI_SSID[] = "Area51";
constexpr char WIFI_PASS[] = "GreenTD70!#%";