// File: include/media/MediaApplication.h

#ifndef MEDIA_APPLICATION_H
#define MEDIA_APPLICATION_H

#include "MediaControllerDevice.h"
#include "RotaryEncoder.h"
#include "TcpServer.h"
#include "btstack.h"
#include "Display.h"
#include "Drawing.h"
#include "FrameProtocol.h"
#include "config.h" 
#include "pico/sync.h"
#include <array>

class MediaApplication; // Forward declaration


class MediaApplication {
public:
    MediaApplication();
    void run();

    // --- Public handler for the protocol dispatcher ---
    void on_image_tile_received(const Protocol::ImageTileHeader& header, const uint8_t* pixel_data, size_t pixel_len);

private:
    MediaControllerDevice m_media_controller;
    RotaryEncoder m_encoder;
    St7789Display m_display;
    
    // Instantiate the templated Drawing class with the buffer size from config.h
    Drawing<MAX_DRAW_BUFFER_PIXELS> m_drawing;
    
    TcpServer m_tcp_server;

    btstack_timer_source_t m_release_timer;
    btstack_timer_source_t m_battery_timer;
    
    uint8_t m_battery_level;
    enum class ButtonState { IDLE, ARMED, PRESSED };
    ButtonState m_button_state;
    uint64_t m_button_armed_time_us;
    
    Drawing<MAX_DRAW_BUFFER_PIXELS>::DrawStatus m_last_draw_status;

    // --- Flag for deferred NACK sending ---
    bool m_nack_is_pending = false;

    // The queue now only needs to hold ONE tile, as TCP flow control
    // will prevent more from arriving until we are ready.
    critical_section_t m_tile_buffer_crit_sec;
    bool m_tile_is_pending = false;
    Protocol::Frame m_pending_tile;

    static void release_handler_forwarder(btstack_timer_source_t* ts);
    static void battery_timer_handler_forwarder(btstack_timer_source_t* ts);
    void release_handler();
    void battery_timer_handler();
};

#endif // MEDIA_APPLICATION_H