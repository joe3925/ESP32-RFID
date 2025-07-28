#pragma once

#include <cstddef>
#include <cstdint>
uint8_t crc8(const uint8_t* d, size_t n){
    uint8_t c = 0x00;
    for(size_t i=0;i<n;i++){
        c ^= d[i];
        for(int b=0;b<8;b++)
            c = (c & 0x80) ? (c<<1) ^ 0x31 : (c<<1);
    }
    return c;
}