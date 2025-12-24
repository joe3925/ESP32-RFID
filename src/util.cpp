// Utility/helper functions.
// Look here for reusable logic that could use clarification.
#include <HardwareSerial.h>
#include <util.hpp>
#include <fpga.hpp>

// Calculates a CRC8 checksum to check if data is correct
// It goes through each byte and scrambles it in a special way to get one number back.
uint8_t crc8(const uint8_t *d, size_t n)
{
    uint8_t c = 0x00;
    for (size_t i = 0; i < n; i++)
    {
        c ^= d[i];
        for (int b = 0; b < 8; b++)
            c = (c & 0x80) ? (c << 1) ^ 0x31 : (c << 1);
    }
    return c;
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
bool wait_byte(HardwareSerial &port, uint8_t &out, uint32_t timeout_ms)
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
                Serial.printf("%02X\n", out);
                return true;
            }
        }
        delay(1);
    }
    return false;
}
const char *status_str(FpgaStatus s)
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
void print_reply(const FpgaReply &r)
{
    Serial.print(F("status="));
    Serial.print(status_str(r.status));
    Serial.print(F(" ("));
    Serial.print(static_cast<uint8_t>(r.status));
    Serial.print(F(") result="));
    Serial.println(r.result);
}