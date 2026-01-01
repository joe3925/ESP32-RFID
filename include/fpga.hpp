#pragma once

#include <HardwareSerial.h>
#include <cstdint>
#include "const.hpp"
struct WakeGuard
{
    int pin;
    WakeGuard(int p) : pin(p)
    {
        digitalWrite(pin, LOW);
        delayMicroseconds(5);
    }
    ~WakeGuard()
    {
        digitalWrite(pin, HIGH);
    }
};

enum class FpgaStatus : uint8_t
{
    Ok,
    TimeoutReady,
    TimeoutResp,
    BadHeader,
    BadCrc
};
enum class FpgaCommand : uint8_t
{
    CMD_CHECK_UID = 0x10,
    CMD_ADD_UID = 0x11,
};
static inline bool fpga_status_ok(FpgaStatus s)
{
    return s == FpgaStatus::Ok;
}

struct FpgaReply
{
    FpgaStatus status;
    uint8_t result;
};

struct AuthReply
{
    bool success;
    FpgaStatus status;
    uint8_t result;

    static AuthReply fromFpgaReply(const FpgaReply& r)
    {
        AuthReply out{};
        out.status = r.status;
        out.result = r.result;
        out.success = fpga_status_ok(r.status) && (r.result == RES_ALLOW);
        return out;
    }
};

struct AddReply
{
    bool success;
    FpgaStatus status;
    uint8_t result;

    static AddReply fromFpgaReply(const FpgaReply& r)
    {
        AddReply out{};
        out.status = r.status;
        out.result = r.result;
        out.success = fpga_status_ok(r.status) &&
                      (r.result == RES_OK || r.result == RES_DUPLICATE);
        return out;
    }
};
FpgaReply fpga_uid_transaction(uint8_t cmd, const uint8_t* uid, uint8_t len);
AuthReply fpga_is_allowed(const uint8_t* uid, uint8_t len);
AddReply fpga_authorize_uid(const uint8_t* uid, uint8_t len);