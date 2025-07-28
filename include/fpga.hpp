#pragma once

#include <cstdint>
#include <HardwareSerial.h>

struct WakeGuard {
    int pin;
    WakeGuard(int p) : pin(p) { digitalWrite(pin, LOW); delayMicroseconds(5); }
    ~WakeGuard() { digitalWrite(pin, HIGH); }
};

enum class FpgaStatus : uint8_t {
    Ok,
    TimeoutReady,
    TimeoutResp,
    BadHeader,
    BadCrc
};
enum class FpgaCommand : uint8_t {
    CMD_CHECK_UID = 0x10,
    CMD_ADD_UID = 0x11, 
};

struct FpgaReply {
    FpgaStatus status;
    uint8_t    result; // RES_ALLOW/RES_DENY/RES_OK/… from FPGA
};
FpgaReply fpga_uid_transaction(uint8_t cmd, const uint8_t* uid, uint8_t len);
bool fpga_is_allowed(const uint8_t* uid, uint8_t len);

bool fpga_authorize_uid(const uint8_t* uid, uint8_t len);