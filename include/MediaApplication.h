// File: MediaApplication.h

#ifndef MEDIA_APPLICATION_H
#define MEDIA_APPLICATION_H

#include "MediaControllerDevice.h"
#include "RotaryEncoder.h"
#include "btstack.h"
#include "Display.h"
#include "Drawing.h"

class MediaApplication {
public:
    MediaApplication();
    void run();

private:
    // We only need ONE generic release handler
    static void release_handler_forwarder(btstack_timer_source_t* ts);
    static void polling_handler_forwarder(btstack_timer_source_t* ts);
    static void battery_timer_handler_forwarder(btstack_timer_source_t* ts);

    void release_handler();
    void polling_handler();
    void battery_timer_handler();

    MediaControllerDevice m_media_controller;
    RotaryEncoder m_encoder;
    
    // We only need ONE generic release timer
    btstack_timer_source_t m_release_timer;
    btstack_timer_source_t m_polling_timer;
    btstack_timer_source_t m_battery_timer;
    
    int m_processed_rotation = 0;
    uint8_t m_battery_level = 100;

    // --- State variables for button debouncing ---
    enum class ButtonState { IDLE, ARMED, PRESSED };
    ButtonState m_button_state = ButtonState::IDLE;
    uint64_t m_button_armed_time_us = 0;

    St7789Display m_display;
    Drawing m_drawing;
};

#endif // MEDIA_APPLICATION_H