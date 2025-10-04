// File: include/net/FrameDispatcher.h (New)
#include "FrameProtocol.h"
#include <functional>

// Forward declare your application
class MediaApplication;

namespace Protocol {
    // Primary template
    template<FrameType F>
    void dispatch(MediaApplication* app, const FrameView& frame);

    // --- Template Specializations for each Frame Type ---

    // Specialization for IMAGE_TILE
    template<>
    inline void dispatch<FrameType::IMAGE_TILE>(MediaApplication* app, const FrameView& frame) {
        // Safely unpack the payload
        ImageTileHeader tile_header;
        memcpy(&tile_header, frame.payload, sizeof(ImageTileHeader));
        
        const uint8_t* pixel_data = frame.payload + sizeof(ImageTileHeader);
        size_t pixel_len = frame.header.payload_length - sizeof(ImageTileHeader);
        
        // Call a new, strongly-typed handler on the app
        app->on_image_tile_received(tile_header, pixel_data, pixel_len);
    }
    
    // ... (specializations for TILE_ACK, etc., if needed) ...

    // The main dispatcher function
    inline void dispatch_frame(MediaApplication* app, const uint8_t* buffer, size_t len) {
        if (len < sizeof(FrameHeader)) return;
        FrameView frame;
        memcpy(&frame.header, buffer, sizeof(FrameHeader));
        frame.payload = buffer + sizeof(FrameHeader);

        switch (frame.header.type) {
            case FrameType::IMAGE_TILE:
                dispatch<FrameType::IMAGE_TILE>(app, frame);
                break;
            // Add other cases here
            default:
                break;
        }
    }
}