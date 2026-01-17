#pragma once

#include <cstddef>
#include <cstdint>
#include <fpga.hpp>
#include <WiFi.hpp>
static inline uint8_t crc8_update_msb_0x07(uint8_t crc, uint8_t data)
{
    crc ^= data;
    for (int i = 0; i < 8; i++)
    {
        if (crc & 0x80)
            crc = (uint8_t)((crc << 1) ^ 0x07);
        else
            crc = (uint8_t)(crc << 1);
    }
    return crc;
}
static inline bool wifi_ok(uint32_t timeout_ms = 1500)
{
    return wifi_test(timeout_ms) == WifiHealth::Ok;
}

enum class FbCheckStatus : uint8_t
{
    Worked,         // Firebase answered; allowed_out is valid (true/false)
    NotAuthed,      // WiFi ok, but Firebase not ready / no token
    PermissionDeny, // Firebase reachable but rules/token deny
    UnknownFail,     // unknown error -> won't fallback
    WifiFail,       // WiFi not available -> will fallback

};
uint8_t fpga_crc8_cmd_len_payload(uint8_t cmd, uint8_t len,
                                  const uint8_t* payload);
bool wait_byte(HardwareSerial& port, uint8_t& out, uint32_t timeout_ms);
const char* status_str(FpgaStatus s);
void print_reply(const FpgaReply& r);