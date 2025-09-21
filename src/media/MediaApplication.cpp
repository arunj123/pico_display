// File: MediaApplication.cpp

#include "MediaApplication.h"
#include "BtStackManager.h"
#include "config.h"
#include <cstdio>
#include "hardware/gpio.h"

static MediaApplication* g_app_instance = nullptr;

static void gpio_irq_handler(uint gpio, uint32_t events) {
    if (g_app_instance) {
        g_app_instance->handle_gpio_irq(gpio, events);
    }
}

MediaApplication::MediaApplication() : 
    m_encoder(pio1, ENCODER_PIN_A)
{
    g_app_instance = this;

    // --- Update timer context setup ---
    m_polling_timer.context = this;
    m_encoder_release_timer.context = this;
    m_button_release_timer.context = this;
    m_battery_timer.context = this;

    m_processed_rotation = m_encoder.get_rotation();

    // --- Update timer handler setup ---
    // The same release handler function can be used for both timers
    btstack_run_loop_set_timer_handler(&m_encoder_release_timer, &MediaApplication::release_handler_forwarder); // <-- RENAMED
    btstack_run_loop_set_timer_handler(&m_button_release_timer, &MediaApplication::release_handler_forwarder);  // <-- ADDED
    btstack_run_loop_set_timer_handler(&m_polling_timer, &MediaApplication::polling_handler_forwarder);
    btstack_run_loop_set_timer_handler(&m_battery_timer, &MediaApplication::battery_timer_handler_forwarder);
    
    // Register the media controller with the BTstack manager
    BtStackManager::getInstance().registerHandler(&m_media_controller);

    // -- Set up the button GPIO ---
    gpio_init(ENCODER_PIN_KEY);
    gpio_set_dir(ENCODER_PIN_KEY, GPIO_IN);
    gpio_pull_up(ENCODER_PIN_KEY);
    
    // Set up the interrupt (only for falling edge - a button press)
    gpio_set_irq_enabled_with_callback(ENCODER_PIN_KEY, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
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

void MediaApplication::handle_gpio_irq(uint gpio, uint32_t events) {
    // We only care about the encoder key pin
    if (gpio != ENCODER_PIN_KEY) return;

    uint32_t current_time_ms = to_ms_since_boot(get_absolute_time());

    // Debounce and set flag. DO NOT CALL OTHER FUNCTIONS.
    if ((current_time_ms - m_last_press_time_ms) > DEBOUNCE_DELAY_MS) {
        m_last_press_time_ms = current_time_ms;
        m_button_pressed_flag = true;
    }
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
    if (app) {
        app->release_handler();
    }
}

void MediaApplication::battery_timer_handler_forwarder(btstack_timer_source_t* ts) {
    MediaApplication* app = static_cast<MediaApplication*>(ts->context);
    if (app) {
        app->battery_timer_handler();
    }
}

// --- Member Function Implementations ---
void MediaApplication::polling_handler() {
    if (m_media_controller.isConnected()) {
        // --- 1. Handle Encoder Rotation ---
        int current_rotation = m_encoder.get_rotation();

        if (current_rotation > m_processed_rotation) {
            printf("Volume Up (Target: %d, Processed: %d)\n", current_rotation, m_processed_rotation);
            m_media_controller.increaseVolume();
            // Use the DEDICATED timer for the encoder
            btstack_run_loop_set_timer(&m_encoder_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_encoder_release_timer);
            m_processed_rotation++;
        } 
        else if (current_rotation < m_processed_rotation) {
            printf("Volume Down (Target: %d, Processed: %d)\n", current_rotation, m_processed_rotation);
            m_media_controller.decreaseVolume();
            // Use the DEDICATED timer for the encoder
            btstack_run_loop_set_timer(&m_encoder_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_encoder_release_timer);
            m_processed_rotation--;
        }

        // --- 2. Handle Button Press ---
        if (m_button_pressed_flag) {
            m_button_pressed_flag = false;
            printf("Mute button pressed\n");
            
            m_media_controller.mute();
            // Use the DEDICATED timer for the button
            btstack_run_loop_set_timer(&m_button_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_button_release_timer);
        }
    }

    // Always reset the polling timer for the next check
    btstack_run_loop_set_timer(&m_polling_timer, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(&m_polling_timer);
}

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