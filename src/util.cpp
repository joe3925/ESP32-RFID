// Utility/helper functions.
// Look here for reusable logic that could use clarification.
#include <HardwareSerial.h>
#include <fpga.hpp>
#include <util.hpp>

// Calculates a CRC8 checksum to check if data is correct
// It goes through each byte and scrambles it in a special way to get one number back.

uint8_t fpga_crc8_cmd_len_payload(uint8_t cmd, uint8_t len,
                                  const uint8_t* payload)
{
    uint8_t crc = 0x00;
    crc = crc8_update_msb_0x07(crc, cmd);
    crc = crc8_update_msb_0x07(crc, len);
    for (uint8_t i = 0; i < len; i++)
        crc = crc8_update_msb_0x07(crc, payload[i]);
    return crc;
}
// bool wait_byte(HardwareSerial &port, uint8_t &out, uint32_t timeout_ms)
// {
//     uint32_t start = millis();
//     // while ((millis() - start) < timeout_ms)
//     while (true)
//     {
//         while (port.available() > 0)
//         {
//             int b = port.read();
//             Serial.printf("%02X\n", b);
//             out = (uint8_t)b;
//             // return true;
//         }
//     }
//     return false;
// }
bool wait_byte(HardwareSerial& port, uint8_t& out, uint32_t timeout_ms)
{
    uint32_t start = millis();
    while ((uint32_t)(millis() - start) < timeout_ms)
    {
        if (port.available() > 0)
        {
            int b = port.read();
            if (b >= 0)
            {
                out = (uint8_t)b;
                return true;
            }
        }
        delay(1);
    }
    return false;
}
const char* status_str(FpgaStatus s)
{
    switch (s)
    {
        case FpgaStatus::Ok:
            return "Ok";
        case FpgaStatus::TimeoutReady:
            return "TimeoutReady";
        case FpgaStatus::TimeoutResp:
            return "TimeoutResp";
        default:
            return "Unknown";
    }
}
void print_reply(const FpgaReply& r)
{
    Serial.print(F("status="));
    Serial.print(status_str(r.status));
    Serial.print(F(" ("));
    Serial.print(static_cast<uint8_t>(r.status));
    Serial.print(F(") result="));
    Serial.println(r.result);
}