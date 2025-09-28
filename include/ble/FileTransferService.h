// File: include/ble/FileTransferService.h

#ifndef FILE_TRANSFER_SERVICE_H
#define FILE_TRANSFER_SERVICE_H

#include <cstdint>

namespace FileTransfer {
    enum class Command : uint8_t {
        START_TRANSFER = 0x01,
        END_TRANSFER   = 0x02,
        ACK_OK         = 0x10,
        ACK_ERROR      = 0x11
    };
    enum class DataType : uint8_t {
        TEXT_UTF8 = 0x01,
        IMAGE_PNG = 0x10,
        IMAGE_JPG = 0x11
    };
}

#endif // FILE_TRANSFER_SERVICE_H
