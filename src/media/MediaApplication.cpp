// File: MediaApplication.cpp

#include "MediaApplication.h"
#include "BtStackManager.h"
#include "config.h"
#include "pico/time.h" // For time_us_64
#include <cstdio>
#include "hardware/gpio.h"
#include "font_freesans_16.h" // generated font
#include "CustomFont.h"
#include "test_img.h"

// Color definitions for convenience
namespace Colors {
    constexpr uint16_t BLACK = 0x0000;
    constexpr uint16_t BLUE = 0x001F;
    constexpr uint16_t RED = 0xF800;
    constexpr uint16_t GREEN = 0x07E0;
    constexpr uint16_t WHITE = 0xFFFF;
}

MediaApplication::MediaApplication() : 
    // FIX 1: Call the correct constructor with all three pins.
    m_encoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_KEY),
    m_display(
        pio1,
        DISPLAY_PIN_SDA,
        DISPLAY_PIN_SCL,
        DISPLAY_PIN_CS,
        DISPLAY_PIN_DC,
        DISPLAY_PIN_RESET,
        DisplayOrientation::PORTRAIT
    ), m_drawing(m_display)
{
    // FIX 2: Remove all GPIO and IRQ setup logic from this class.
    // The RotaryEncoder's constructor now handles this automatically.
    
    m_polling_timer.context = this;
    m_release_timer.context = this;
    m_battery_timer.context = this;
    
    btstack_run_loop_set_timer_handler(&m_release_timer, &MediaApplication::release_handler_forwarder);
    btstack_run_loop_set_timer_handler(&m_polling_timer, &MediaApplication::polling_handler_forwarder);
    btstack_run_loop_set_timer_handler(&m_battery_timer, &MediaApplication::battery_timer_handler_forwarder);
    
    BtStackManager::getInstance().registerHandler(&m_media_controller);

    m_display.fillScreen(Colors::BLACK);
    m_drawing.fillRect(10, 10, m_drawing.getWidth() - 20, 30, Colors::RED);
    m_drawing.drawRect(9, 9, m_drawing.getWidth() - 18, 32, Colors::WHITE);
    m_drawing.drawImage(
        (m_drawing.getWidth() - test_img_width) / 2,
        (m_drawing.getHeight() - test_img_height) / 2,
        test_img_width,
        test_img_height,
        test_img_data
    );
    m_drawing.drawString(20, 20, "Hello, Display!", Colors::WHITE, &font_freesans_16);
    m_drawing.drawLine(20, 50, 300, 50, Colors::GREEN);
}

void MediaApplication::run() {
    m_media_controller.setup();
    m_media_controller.powerOn();

    // Start the main polling timer
    btstack_run_loop_set_timer(&m_polling_timer, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(&m_polling_timer);

    // Start the battery update timer (e.g., update every 30 seconds)
    btstack_run_loop_set_timer(&m_battery_timer, 30000);
    btstack_run_loop_add_timer(&m_battery_timer);

    // This is a blocking call that starts the event loop
    btstack_run_loop_execute();
}



// --- Static Forwarders to bridge C-style callbacks to C++ methods ---
void MediaApplication::polling_handler_forwarder(btstack_timer_source_t* ts) {
    MediaApplication* app = static_cast<MediaApplication*>(ts->context);
    if (app) {
        app->polling_handler();
    }
}

void MediaApplication::release_handler_forwarder(btstack_timer_source_t* ts) {
    MediaApplication* app = static_cast<MediaApplication*>(ts->context);
    if (app) app->release_handler();
}

void MediaApplication::battery_timer_handler_forwarder(btstack_timer_source_t* ts) {
    MediaApplication* app = static_cast<MediaApplication*>(ts->context);
    if (app) {
        app->battery_timer_handler();
    }
}

// --- Member Function Implementations ---
// --- Polling Handler ---
void MediaApplication::polling_handler() {
    if (m_media_controller.isConnected()) {

        // --- Handle Rotation (Unchanged) ---
        int8_t rotation_delta = m_encoder.read_and_clear_rotation();
        if (rotation_delta > 0) {
            printf("Volume Up\n");
            m_media_controller.increaseVolume();
            btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_release_timer);
        } else if (rotation_delta < 0) {
            printf("Volume Down\n");
            m_media_controller.decreaseVolume();
            btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_release_timer);
        }

        // --- Handle Key Press with a Polling-Based Debounce State Machine ---
        uint64_t now_us = time_us_64();
        bool is_key_down = (gpio_get(ENCODER_PIN_KEY) == 0);

        switch (m_button_state) {
            case ButtonState::IDLE:
                if (is_key_down) {
                    // Pin has gone low. Start the debounce timer and move to ARMED.
                    m_button_armed_time_us = now_us;
                    m_button_state = ButtonState::ARMED;
                }
                break;
            
            case ButtonState::ARMED:
                if (!is_key_down) {
                    // Button was released before debounce time, it was a glitch. Reset.
                    m_button_state = ButtonState::IDLE;
                } else if ((now_us - m_button_armed_time_us) > (DEBOUNCE_DELAY_MS_KEY * 1000)) {
                    // Button has been held down for the debounce duration. It's a valid press.
                    printf("Mute button pressed\n");
                    m_media_controller.mute();
                    btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
                    btstack_run_loop_add_timer(&m_release_timer);
                    
                    // Move to PRESSED state to wait for the button to be released.
                    m_button_state = ButtonState::PRESSED;
                }
                break;
            
            case ButtonState::PRESSED:
                if (!is_key_down) {
                    // Button has been released. Reset for the next press.
                    m_button_state = ButtonState::IDLE;
                }
                break;
        }
    }

    // Always reset the polling timer
    btstack_run_loop_set_timer(&m_polling_timer, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(&m_polling_timer);
}


// --- Single, generic release handler ---
void MediaApplication::release_handler() {
    if (m_media_controller.isConnected()) {
        m_media_controller.release();
    }
}

void MediaApplication::battery_timer_handler() {
    if (m_battery_level > 0) {
        m_battery_level--;
    }

    printf("Updating battery level to %d%%\n", m_battery_level);
    
    // Call our new, clean method on the media controller object
    m_media_controller.setBatteryLevel(m_battery_level);

    // Reset the timer for the next update
    btstack_run_loop_set_timer(&m_battery_timer, 30000);
    btstack_run_loop_add_timer(&m_battery_timer);
}