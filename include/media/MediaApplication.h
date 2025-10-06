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
#include <cassert>
#include <array>

class MediaApplication {
public:
    MediaApplication();
    void setup();
    // This is the correct signature for the callback
    void on_image_tile_received(const Protocol::FrameHeader& frame_header, const uint8_t* payload);
    void on_valid_tile_received(const Protocol::FrameHeader& frame_header, const uint8_t* payload);

private:
    void handle_encoder();
    void poll_handler();
    static void poll_handler_forwarder(btstack_timer_source_t* ts);

    MediaControllerDevice m_media_controller;
    RotaryEncoder m_encoder;
    St7789Display m_display;
    Drawing m_drawing;
    TcpServer m_tcp_server;

    btstack_timer_source_t m_poll_timer;
    btstack_timer_source_t m_release_timer;
    btstack_timer_source_t m_battery_timer;
    
    uint8_t m_battery_level;
    enum class ButtonState { IDLE, ARMED, PRESSED };
    ButtonState m_button_state;
    uint64_t m_button_armed_time_us;
    
    critical_section_t m_tile_queue_crit_sec;
    static constexpr size_t TILE_QUEUE_SIZE = 4;
    std::array<Protocol::Frame, TILE_QUEUE_SIZE> m_tile_queue;
    volatile uint8_t m_queue_head = 0;
    volatile uint8_t m_queue_tail = 0;

    static void release_handler_forwarder(btstack_timer_source_t* ts);
    static void battery_timer_handler_forwarder(btstack_timer_source_t* ts);
    void release_handler();
    void battery_timer_handler();
};

#endif // MEDIA_APPLICATION_H