// File: include/net/FrameProtocol.h

#ifndef FRAME_PROTOCOL_H
#define FRAME_PROTOCOL_H

#include <cstdint>
#include <array>

// Use pragma pack to ensure the compiler doesn't add padding bytes.
#pragma pack(push, 1)

namespace Protocol {

    constexpr uint8_t FRAME_MAGIC = 0xAA;
    
    // Maximum possible size for any frame's payload.
    // From config.py: TILE_PAYLOAD_SIZE = 8192
    constexpr size_t MAX_PAYLOAD_SIZE = 8192;

    enum class FrameType : uint8_t {
        THROUGHPUT_TEST = 0x01,
        IMAGE_TILE      = 0x02,
        TILE_ACK        = 0x03,
        TILE_NACK       = 0x04
    };

    struct FrameHeader {
        uint8_t   magic;
        FrameType type;
        uint16_t  payload_length;
    };

    struct ImageTileHeader {
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
        uint32_t crc32;
    };

    // A structure to hold a complete, parsed frame using a fixed-size buffer
    struct Frame {
        FrameHeader header;
        std::array<uint8_t, MAX_PAYLOAD_SIZE> payload;
    };

} // namespace Protocol

#pragma pack(pop)

#endif // FRAME_PROTOCOL_H