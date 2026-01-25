#ifndef PROTO_ATT_H
#define PROTO_ATT_H

#include <cstdint>
#include <vector>
#include <span>

namespace ble {

// ATT Opcodes
constexpr uint8_t ATT_OP_ERROR_RESP            = 0x01;
constexpr uint8_t ATT_OP_EXCHANGE_MTU_REQ      = 0x02;
constexpr uint8_t ATT_OP_EXCHANGE_MTU_RESP     = 0x03;
constexpr uint8_t ATT_OP_FIND_INFO_REQ         = 0x04;
constexpr uint8_t ATT_OP_FIND_INFO_RESP        = 0x05;
constexpr uint8_t ATT_OP_READ_BY_TYPE_REQ      = 0x08;
constexpr uint8_t ATT_OP_READ_BY_TYPE_RESP     = 0x09;
constexpr uint8_t ATT_OP_READ_REQ              = 0x0A;
constexpr uint8_t ATT_OP_READ_RESP             = 0x0B;
constexpr uint8_t ATT_OP_READ_GROUP_REQ        = 0x10;
constexpr uint8_t ATT_OP_READ_GROUP_RESP       = 0x11;
constexpr uint8_t ATT_OP_WRITE_REQ             = 0x12;
constexpr uint8_t ATT_OP_WRITE_RESP            = 0x13;
constexpr uint8_t ATT_OP_WRITE_CMD             = 0x52;

// UUID (16-bit and 128-bit)
struct UUID {
    uint8_t type = 0; // 0=16, 1=128
    uint16_t u16 = 0;
    uint8_t u128[16] = {0};

    bool operator==(const UUID& other) const {
        if (type != other.type) return false;
        if (type == 0) return u16 == other.u16;
        for(int i=0; i<16; i++) if (u128[i] != other.u128[i]) return false;
        return true;
    }
};

// Error Codes
constexpr uint8_t ATT_ECODE_INVALID_HANDLE     = 0x01;
constexpr uint8_t ATT_ECODE_READ_NOT_PERM      = 0x02;
constexpr uint8_t ATT_ECODE_WRITE_NOT_PERM     = 0x03;
constexpr uint8_t ATT_ECODE_INVALID_PDU        = 0x04;
constexpr uint8_t ATT_ECODE_REQ_NOT_SUPP       = 0x06;
constexpr uint8_t ATT_ECODE_ATTR_NOT_FOUND     = 0x0A;

} // ble

#endif // PROTO_ATT_H
