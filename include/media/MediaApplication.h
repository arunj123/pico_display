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

// The global callback that TcpServer will use
void on_frame_received_callback(MediaApplication* app, const uint8_t* data, size_t len);

class MediaApplication {
public:
    MediaApplication();
    void run();

    // --- Public handler for the protocol dispatcher ---
    void on_image_tile_received(const Protocol::ImageTileHeader& header, const uint8_t* pixel_data, size_t pixel_len);

private:
    // Helper to push a validated frame into our private queue
    void push_tile_to_queue(const Protocol::Frame& frame);

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

    // A fixed-size circular buffer for incoming image tiles
    critical_section_t m_tile_queue_crit_sec;
    static constexpr size_t TILE_QUEUE_SIZE = 4;
    std::array<Protocol::Frame, TILE_QUEUE_SIZE> m_tile_queue;
    uint8_t m_queue_head = 0;
    uint8_t m_queue_tail = 0;
    uint8_t m_queue_count = 0;

    static void release_handler_forwarder(btstack_timer_source_t* ts);
    static void battery_timer_handler_forwarder(btstack_timer_source_t* ts);
    void release_handler();
    void battery_timer_handler();
};

#endif // MEDIA_APPLICATION_H