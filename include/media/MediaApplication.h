// File: include/media/MediaApplication.h

#ifndef MEDIA_APPLICATION_H
#define MEDIA_APPLICATION_H

#include "MediaControllerDevice.h"
#include "RotaryEncoder.h"
#include "btstack.h"
#include "Display.h"
#include "Drawing.h"
#include "FileTransferService.h"

class MediaApplication {
public:
    MediaApplication();
    void run();
    MediaControllerDevice& getMediaController() { return m_media_controller; }

private:
    // --- Member Variables MUST be declared before member functions ---
    MediaControllerDevice m_media_controller;
    RotaryEncoder m_encoder;
    St7789Display m_display;
    Drawing m_drawing;

    btstack_timer_source_t m_release_timer;
    btstack_timer_source_t m_polling_timer;
    btstack_timer_source_t m_battery_timer;
    
    uint8_t m_battery_level;

    enum class ButtonState { IDLE, ARMED, PRESSED };
    ButtonState m_button_state;
    uint64_t m_button_armed_time_us;

    // --- Static forwarders for C-style callbacks ---
    static void release_handler_forwarder(btstack_timer_source_t* ts);
    static void polling_handler_forwarder(btstack_timer_source_t* ts);
    static void battery_timer_handler_forwarder(btstack_timer_source_t* ts);

    // --- Member function handlers ---
    void release_handler();
    void polling_handler();
    void battery_timer_handler();
};

#endif // MEDIA_APPLICATION_H
