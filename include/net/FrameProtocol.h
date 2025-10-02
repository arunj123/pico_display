#ifndef FRAME_PROTOCOL_H
#define FRAME_PROTOCOL_H

#include <cstdint>
#include <vector>

// Use pragma pack to ensure the compiler doesn't add padding bytes,
// which would break the protocol between the C++ and Python sides.
#pragma pack(push, 1)

constexpr uint8_t FRAME_MAGIC = 0xAA;

enum FrameType : uint8_t {
    THROUGHPUT_TEST = 0x01,
    IMAGE_TILE      = 0x02
};

struct FrameHeader {
    uint8_t  magic;
    FrameType type;
    uint16_t payload_length;
};

// The payload for an IMAGE_TILE frame starts with this header
struct ImageTileHeader {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
};

// A structure to hold a complete, parsed frame
struct Frame {
    FrameHeader header;
    std::vector<uint8_t> payload;
};

#pragma pack(pop)

#endif // FRAME_PROTOCOL_H