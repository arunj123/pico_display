// File: include/media/MediaApplication.h

#ifndef MEDIA_APPLICATION_H
#define MEDIA_APPLICATION_H

#include "MediaControllerDevice.h"
#include "RotaryEncoder.h"
#include "TcpServer.h"
#include "btstack.h"
#include "Display.h"
#include "Drawing.h"
#include <vector>
#include "pico/sync.h"
#include "FrameProtocol.h" // Make sure this is included

void on_frame_received(const Frame& frame);

enum class TileRxStatus {
    NONE,
    ACK_PENDING,
    NACK_PENDING
};

class MediaApplication {
public:
    MediaApplication();
    void run();
    TcpServer& getTcpServer() { return m_tcp_server; }
    MediaControllerDevice& getMediaController() { return m_media_controller; }
    Drawing& getDrawing() { return m_drawing; }
    void push_tile_to_queue(const Frame& frame);
    void set_tile_rx_status(TileRxStatus status);

private:
    MediaControllerDevice m_media_controller;
    RotaryEncoder m_encoder;
    St7789Display m_display;
    Drawing m_drawing;
    TcpServer m_tcp_server;

    btstack_timer_source_t m_release_timer;
    btstack_timer_source_t m_battery_timer;
    
    uint8_t m_battery_level;
    enum class ButtonState { IDLE, ARMED, PRESSED };
    ButtonState m_button_state;
    uint64_t m_button_armed_time_us;

    // A queue for incoming image tiles
    critical_section_t m_tile_queue_crit_sec;
    std::vector<Frame> m_tile_queue;
    volatile TileRxStatus m_tile_rx_status;

    // Static forwarders
    static void release_handler_forwarder(btstack_timer_source_t* ts);
    static void battery_timer_handler_forwarder(btstack_timer_source_t* ts);
    void release_handler();
    void battery_timer_handler();
};

#endif