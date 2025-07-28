#include <HardwareSerial.h>
#include <util.hpp>
uint8_t crc8(const uint8_t* d, size_t n){
    uint8_t c = 0x00;
    for(size_t i=0;i<n;i++){
        c ^= d[i];
        for(int b=0;b<8;b++)
            c = (c & 0x80) ? (c<<1) ^ 0x31 : (c<<1);
    }
    return c;
}
 bool wait_byte(HardwareSerial& port, uint8_t& out, uint32_t timeout_ms) {
    uint32_t start = millis();
    while ((millis() - start) < timeout_ms) {
        if (port.available()) {
            out = static_cast<uint8_t>(port.read());
            return true;
        }
    }
    return false;
}