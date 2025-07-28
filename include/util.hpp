#pragma once

#include <cstddef>
#include <cstdint>
uint8_t crc8(const uint8_t* d, size_t n);
 bool wait_byte(HardwareSerial& port, uint8_t& out, uint32_t timeout_ms);