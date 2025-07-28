#pragma once
#include <cstdint>

enum class WifiHealth : uint8_t {
    NotConnected,   
    DnsFail,        
    TcpFail,        
    Ok
};

bool wifi_connect(uint32_t timeout_ms);

WifiHealth wifi_test(uint32_t timeout_ms);


