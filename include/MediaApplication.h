// File: MediaApplication.h

#ifndef MEDIA_APPLICATION_H
#define MEDIA_APPLICATION_H

#include "MediaControllerDevice.h"
#include "RotaryEncoder.h"
#include "btstack.h"

class MediaApplication {
public:
    MediaApplication();
    void run();

    // Public method to be called from the C-style IRQ handler
    void handle_gpio_irq(uint gpio, uint32_t events);

private:
    // Timer callback handlers (must be static for C-style callbacks)
    static void polling_handler_forwarder(btstack_timer_source_t* ts);
    static void release_handler_forwarder(btstack_timer_source_t* ts);
    static void battery_timer_handler_forwarder(btstack_timer_source_t* ts);

    // Actual implementation of handlers
    void polling_handler();
    void release_handler();
    void battery_timer_handler();

    MediaControllerDevice m_media_controller;
    RotaryEncoder m_encoder;
    
    btstack_timer_source_t m_encoder_release_timer;
    btstack_timer_source_t m_button_release_timer;
    btstack_timer_source_t m_polling_timer;
    btstack_timer_source_t m_battery_timer;
    
    int m_processed_rotation = 0;
    uint8_t m_battery_level = 100;
    uint32_t m_last_press_time_ms = 0;
    volatile bool m_button_pressed_flag = false;
};

#endif // MEDIA_APPLICATION_H