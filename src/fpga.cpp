#include <const.hpp>
#include <fpga.hpp>
#include <util.hpp>

// Handles FPGA communication

// ============================================================================
// Physical Lines
// ----------------------------------------------------------------------------
//   FPGA_WAKE : ESP32 -> FPGA, active LOW (wake/interrupt request)
//   UART      : Bi-directional (ESP32 Serial2 <-> FPGA UART)
//
// ============================================================================
// Handshake / Timing
// ----------------------------------------------------------------------------
// 1. ESP32 drives FPGA_WAKE LOW, waits for READY within T_READY_MS.
// 2. FPGA sends READY byte (0x52).
// 3. ESP32 sends one FRAME.
// 4. FPGA replies with a single RESULT byte (or extended reply, see below).
// 5. ESP32 drives FPGA_WAKE HIGH.
//
// Timeouts:
//   READY_TIMEOUT_MS  : max wait for READY after pulling FPGA_WAKE low
//   RESP_TIMEOUT_MS   : max wait for RESULT after sending the frame
//
// ============================================================================
// Frame: ESP32 -> FPGA
// ----------------------------------------------------------------------------
//   MAGIC  CMD  LEN  PAYLOAD[LEN]  CRC8
//
//   MAGIC   : 0xA5 (frame delimiter)
//   CMD     : Command code (see below)
//   LEN     : Number of payload bytes (0..255)
//   PAYLOAD : Command-specific data (e.g., UID bytes)
//   CRC8    : CRC-8 of CMD|LEN|PAYLOAD (MAGIC excluded)
//
// ============================================================================
// Commands (CMD)
// ----------------------------------------------------------------------------
//   0x10  CMD_CHECK_UID   Payload: UID bytes
//         Ask FPGA if UID is authorized.
//
//   0x11  CMD_ADD_UID     Payload: UID bytes
//         Request FPGA to add/authorize this UID.
//
// ============================================================================
// Reply: FPGA -> ESP32
// ----------------------------------------------------------------------------
// Simple (1-byte) reply for the two commands above:
//
//   RESULT
//
//   For CMD_CHECK_UID:
//     0x01 = allow
//     0x00 = deny
//
//   For CMD_ADD_UID:
//     0x02 = ok (added)
//     0xEE = duplicate / already present
//     0xEF = storage full / error
//

// This function sends a command and UID to FPGA and waits for a reply.

FpgaReply fpga_uid_transaction(FpgaCommand cmd, const uint8_t* uid, uint8_t len)
{

    WakeGuard wg(FPGA_WAKE);                     // Wake FPGA (pull LOW)
    while (Serial2.available()) Serial2.read();  // Clear old data

    uint8_t b;
    if (!wait_byte(Serial2, b, READY_TIMEOUT_MS))
    {
        return {FpgaStatus::TimeoutReady, 0};
    }
    Serial.printf("Ready byte response: 0x%02X\n", b);
    if (b != FPGA_READY)
    {
        return {FpgaStatus::TimeoutReady, 0};
    }

    uint8_t crc = fpga_crc8_cmd_len_payload((uint8_t)cmd, len, uid);

    while (Serial2.available())
    {
        Serial2.read();
    }

    Serial2.write(FRAME_MAGIC);
    Serial2.write((uint8_t)cmd);
    Serial2.write(len);
    Serial2.write(uid, len);
    Serial2.write(crc);

    uint8_t resp;
    if (!wait_byte(Serial2, resp, RESP_TIMEOUT_MS))
    {
        return {FpgaStatus::TimeoutResp, 0};
    }

    Serial.printf("Response 0x%02X\n", resp);
    while (Serial2.available())
    {
        uint8_t byte = Serial2.read();
    }

    Serial.println(F("fpga ok"));
    return {FpgaStatus::Ok, resp};
}

AuthReply fpga_is_allowed(const uint8_t* uid, uint8_t len)
{
    auto r = fpga_uid_transaction(FpgaCommand::CMD_CHECK_UID, uid, len);
    print_reply(r);
    return AuthReply::fromFpgaReply(r);
}

AddReply fpga_authorize_uid(const uint8_t* uid, uint8_t len)
{
    auto r = fpga_uid_transaction(FpgaCommand::CMD_ADD_UID, uid, len);
    print_reply(r);
    return AddReply::fromFpgaReply(r);
}