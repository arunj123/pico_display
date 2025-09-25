// File: MediaApplication.cpp

#include "MediaApplication.h"
#include "BtStackManager.h"
#include "config.h"
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

static MediaApplication* g_app_instance = nullptr;

static void gpio_irq_handler(uint gpio, uint32_t events) {
    if (g_app_instance) {
        g_app_instance->handle_gpio_irq(gpio, events);
    }
}

MediaApplication::MediaApplication() : 
    // Update constructor to pass pin numbers instead of PIO instance
    m_encoder(ENCODER_PIN_A, ENCODER_PIN_B),
    m_display(
        pio1,
        DISPLAY_PIN_SDA,
        DISPLAY_PIN_SCL,
        DISPLAY_PIN_CS,
        DISPLAY_PIN_DC,
        DISPLAY_PIN_RESET,
        DisplayOrientation::PORTRAIT // Default to the display's native orientation
    ), m_drawing(m_display)
{
    g_app_instance = this;

    // --- Simplified timer setup ---
    m_polling_timer.context = this;
    m_release_timer.context = this;
    m_battery_timer.context = this;
    
    m_processed_rotation = m_encoder.get_rotation();

    // --- Set up the single, generic release handler ---
    btstack_run_loop_set_timer_handler(&m_release_timer, &MediaApplication::release_handler_forwarder);
    btstack_run_loop_set_timer_handler(&m_polling_timer, &MediaApplication::polling_handler_forwarder);
    btstack_run_loop_set_timer_handler(&m_battery_timer, &MediaApplication::battery_timer_handler_forwarder);
    
    // Register the media controller with the BTstack manager
    BtStackManager::getInstance().registerHandler(&m_media_controller);

    // --- UPDATED INTERRUPT SETUP ---
    // Button setup remains the same
    gpio_init(ENCODER_PIN_KEY);
    gpio_set_dir(ENCODER_PIN_KEY, GPIO_IN);
    gpio_pull_up(ENCODER_PIN_KEY);
    
    // Enable interrupts for the button on falling edge
    gpio_set_irq_enabled(ENCODER_PIN_KEY, GPIO_IRQ_EDGE_FALL, true);

    // Enable interrupts for encoder pins A and B on both edges
    gpio_set_irq_enabled(ENCODER_PIN_A, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ENCODER_PIN_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    
    // Set the single shared callback function
    gpio_set_irq_callback(&gpio_irq_handler);
    irq_set_enabled(IO_IRQ_BANK0, true); // Enable interrupts on BANK0

    m_display.fillScreen(Colors::BLACK);

    m_drawing.fillRect(10, 10, m_drawing.getWidth() - 20, 30, Colors::RED);
    m_drawing.drawRect(9, 9, m_drawing.getWidth() - 18, 32, Colors::WHITE);
    
    m_drawing.drawImage(
        (m_drawing.getWidth() - test_img_width) / 2, // Center horizontally
        (m_drawing.getHeight() - test_img_height) / 2, // Center vertically
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

void MediaApplication::handle_gpio_irq(uint gpio, uint32_t events) {
    if (gpio == ENCODER_PIN_KEY && (events & GPIO_IRQ_EDGE_FALL)) {
        // Handle button press with debouncing
        uint32_t current_time_ms = to_ms_since_boot(get_absolute_time());
        if ((current_time_ms - m_last_press_time_ms) > DEBOUNCE_DELAY_MS) {
            m_last_press_time_ms = current_time_ms;
            m_button_pressed_flag = true;
        }
    } else if (gpio == ENCODER_PIN_A || gpio == ENCODER_PIN_B) {
        // Handle encoder rotation
        m_encoder.process_state_change();
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
        int current_rotation = m_encoder.get_rotation();
        int delta = current_rotation - m_processed_rotation;

        if (delta >= ENCODER_COUNTS_PER_STEP) {
            printf("Volume Up (Target: %d, Processed: %d)\n", current_rotation, m_processed_rotation);
            m_media_controller.increaseVolume();
            btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_release_timer);
            m_processed_rotation += ENCODER_COUNTS_PER_STEP;
        } 
        else if (delta <= -ENCODER_COUNTS_PER_STEP) {
            printf("Volume Down (Target: %d, Processed: %d)\n", current_rotation, m_processed_rotation);
            m_media_controller.decreaseVolume();
            btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_release_timer);
            m_processed_rotation -= ENCODER_COUNTS_PER_STEP;
        }

        if (m_button_pressed_flag) {
            m_button_pressed_flag = false;
            printf("Mute button pressed\n");
            m_media_controller.mute();
            btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_release_timer);
        }
    }

    // Always reset the polling timer for the next check
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