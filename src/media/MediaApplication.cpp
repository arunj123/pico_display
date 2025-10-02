// File: src/media/MediaApplication.cpp

#include "MediaApplication.h"
#include "BtStackManager.h"
#include "config.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "font_freesans_16.h"
#include "test_img.h"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "lwip/ip4_addr.h"
#include "FrameProtocol.h"

MediaApplication* g_app_instance = nullptr;

namespace Colors {
    constexpr uint16_t BLACK = 0x0000;
    constexpr uint16_t RED   = 0xF800;
    constexpr uint16_t WHITE = 0xFFFF;
    constexpr uint16_t GREEN = 0x07E0;
}

// Add this function near the top of the file, after the includes.
void debug_print_gpio_status(uint gpio) {
    printf("--- GPIO %d Status ---\n", gpio);
    
    // Get the function select
    printf("  Function: %d (SIO=%d)\n", gpio_get_function(gpio), GPIO_FUNC_SIO);
    
    // Get pull-up/pull-down status
    printf("  Pull Up: %d, Pull Down: %d\n", gpio_is_pulled_up(gpio), gpio_is_pulled_down(gpio));

    // Get interrupt enabled status for this core
    io_bank0_irq_ctrl_hw_t *irq_ctrl_base = &io_bank0_hw->proc0_irq_ctrl;
    uint32_t events = (irq_ctrl_base->inte[gpio / 8] >> (4 * (gpio % 8))) & 0xf;
    
    printf("  IRQ Enabled (This Core):\n");
    printf("    LEVEL_LOW:  %d\n", (events & GPIO_IRQ_LEVEL_LOW) ? 1 : 0);
    printf("    LEVEL_HIGH: %d\n", (events & GPIO_IRQ_LEVEL_HIGH) ? 1 : 0);
    printf("    EDGE_FALL:  %d\n", (events & GPIO_IRQ_EDGE_FALL) ? 1 : 0);
    printf("    EDGE_RISE:  %d\n", (events & GPIO_IRQ_EDGE_RISE) ? 1 : 0);
    printf("--------------------\n");
}

// --- NEW FRAME-BASED CALLBACK ---
void on_frame_received(const Frame& frame) {
    if (!g_app_instance) return;

    switch(frame.header.type) {
        case THROUGHPUT_TEST:
            printf("THROUGHPUT_TEST frame received, echoing back.\n");
            g_app_instance->getTcpServer().send_frame(THROUGHPUT_TEST, frame.payload);
            break;

        case IMAGE_TILE:
        {
            // The DMA/PIO drawing is non-blocking. While the DMA is busy sending
            // pixel data to the display, the CPU is free to return to the main loop
            // and process the next incoming TCP packet. This creates a natural
            // pipeline and acts as our double-buffering mechanism.
            ImageTileHeader tile_header;
            memcpy(&tile_header, frame.payload.data(), sizeof(ImageTileHeader));
            
            const uint16_t* pixel_data = reinterpret_cast<const uint16_t*>(frame.payload.data() + sizeof(ImageTileHeader));
            
            printf("IMAGE_TILE received: x=%u, y=%u, w=%u, h=%u\n", 
                    tile_header.x, tile_header.y, tile_header.width, tile_header.height);
            
            g_app_instance->getDrawing().drawImage(tile_header.x, tile_header.y, tile_header.width, tile_header.height, pixel_data);
            break;
        }
            
        default:
            printf("Unknown frame type received: 0x%02x\n", frame.header.type);
            break;
    }
}

// Constructor: Re-enable display and drawing members
MediaApplication::MediaApplication() :
    m_encoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_KEY),
    m_display(pio1, DISPLAY_PIN_SDA, DISPLAY_PIN_SCL, DISPLAY_PIN_CS, DISPLAY_PIN_DC, DISPLAY_PIN_RESET, DisplayOrientation::PORTRAIT),
    m_drawing(m_display),
    m_battery_level(100),
    m_button_state(ButtonState::IDLE),
    m_button_armed_time_us(0)
{
    g_app_instance = this;

    m_polling_timer.context = this;
    m_release_timer.context = this;
    m_battery_timer.context = this;
    
    btstack_run_loop_set_timer_handler(&m_release_timer, &MediaApplication::release_handler_forwarder);
    btstack_run_loop_set_timer_handler(&m_polling_timer, &MediaApplication::polling_handler_forwarder);
    btstack_run_loop_set_timer_handler(&m_battery_timer, &MediaApplication::battery_timer_handler_forwarder);
    
    BtStackManager::getInstance().registerHandler(&m_media_controller);
    m_tcp_server.setReceiveCallback(on_frame_received);

    // NOTE: All drawing calls have been moved out of the constructor.
}

void MediaApplication::run() {
    printf("Initializing Rotary Encoder...\n");
    m_encoder.init();
    sleep_ms(50);
    m_encoder.read_and_clear_rotation();

    // --- THE FIX: Initialize display and draw the UI here ---
    printf("Initializing Display...\n");
    m_display.init();
    
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
    
    // --- End of new display code ---

    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi network: %s\n", WIFI_SSID);

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Failed to connect to Wi-Fi.\n");
    } else {
        printf("Connected to Wi-Fi. IP: %s\n", ip4addr_ntoa(netif_ip4_addr(&cyw43_state.netif[0])));
        m_tcp_server.init(4242);
    }
    
    printf("Initializing BTstack...\n");
    m_media_controller.setup();
    m_media_controller.powerOn();
    
    btstack_run_loop_set_timer(&m_polling_timer, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(&m_polling_timer);
    btstack_run_loop_set_timer(&m_battery_timer, 30000);
    btstack_run_loop_add_timer(&m_battery_timer);

    while (true) {
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
}

// --- Static Forwarders ---
void MediaApplication::release_handler_forwarder(btstack_timer_source_t* ts) { static_cast<MediaApplication*>(ts->context)->release_handler(); }
void MediaApplication::polling_handler_forwarder(btstack_timer_source_t* ts) { static_cast<MediaApplication*>(ts->context)->polling_handler(); }
void MediaApplication::battery_timer_handler_forwarder(btstack_timer_source_t* ts) { static_cast<MediaApplication*>(ts->context)->battery_timer_handler(); }

// --- Member Handlers ---
void MediaApplication::polling_handler() {
    if (m_media_controller.isConnected()) {
        int8_t rotation_delta = m_encoder.read_and_clear_rotation();
        
        // --- DEBUG: Add this line ---
        if (rotation_delta != 0) {
            printf("Polling found rotation_delta: %d\n", rotation_delta);
        }

        if (rotation_delta > 0) {
            m_media_controller.increaseVolume();
            btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_release_timer);
        } else if (rotation_delta < 0) {
            m_media_controller.decreaseVolume();
            btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_release_timer);
        }

        uint64_t now_us = time_us_64();
        bool is_key_down = (gpio_get(ENCODER_PIN_KEY) == 0);

        switch (m_button_state) {
            case ButtonState::IDLE:
                if (is_key_down) {
                    m_button_armed_time_us = now_us;
                    m_button_state = ButtonState::ARMED;
                }
                break;
            case ButtonState::ARMED:
                if (!is_key_down) {
                    m_button_state = ButtonState::IDLE;
                } else if ((now_us - m_button_armed_time_us) > (DEBOUNCE_DELAY_MS_KEY * 1000)) {
                    m_media_controller.mute();
                    btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
                    btstack_run_loop_add_timer(&m_release_timer);
                    m_button_state = ButtonState::PRESSED;
                }
                break;
            case ButtonState::PRESSED:
                if (!is_key_down) {
                    m_button_state = ButtonState::IDLE;
                }
                break;
        }
    }
    btstack_run_loop_set_timer(&m_polling_timer, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(&m_polling_timer);
}

void MediaApplication::release_handler() {
    if (m_media_controller.isConnected()) {
        m_media_controller.release();
    }
}

void MediaApplication::battery_timer_handler() {
    if (m_battery_level > 0) m_battery_level--;
    m_media_controller.setBatteryLevel(m_battery_level);
    btstack_run_loop_set_timer(&m_battery_timer, 30000);
    btstack_run_loop_add_timer(&m_battery_timer);
}
