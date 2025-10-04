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
#include "pico/stdlib.h"


// Pre-computed CRC32 lookup table. Standard IEEE 802.3 CRC32 Software Implementation ---

// This pre-computed table is for the standard CRC-32 algorithm (as used in PNG, Ethernet)
static const uint32_t crc32_table[256] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887c, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713c, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4c,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};

uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xffffffff;
    for (size_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ^ 0xffffffff;
}
// --- End CRC32 ---

// Global instance pointer for the callback
MediaApplication* g_app_instance = nullptr;

// The global callback that TcpServer invokes. It then calls a member function.
void on_frame_received_callback(MediaApplication* app, const uint8_t* data, size_t len) {
    if (!app || len < sizeof(Protocol::FrameHeader)) return;
    
    Protocol::FrameHeader header;
    memcpy(&header, data, sizeof(header));
    const uint8_t* payload = data + sizeof(header);
    
    if (header.type == Protocol::FrameType::IMAGE_TILE) {
        if (header.payload_length < sizeof(Protocol::ImageTileHeader)) return;
        
        Protocol::ImageTileHeader tile_header;
        memcpy(&tile_header, payload, sizeof(tile_header));
        
        const uint8_t* pixel_data = payload + sizeof(tile_header);
        size_t pixel_len = header.payload_length - sizeof(tile_header);
        
        app->on_image_tile_received(tile_header, pixel_data, pixel_len);
    }
}

// In the constructor
MediaApplication::MediaApplication() : 
    m_encoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_PIN_KEY),
    m_display(pio1, DISPLAY_PIN_SDA, DISPLAY_PIN_SCL, DISPLAY_PIN_CS, DISPLAY_PIN_DC, DISPLAY_PIN_RESET, DisplayOrientation::LANDSCAPE),
    m_drawing(m_display),
    m_tcp_server(this), // Pass self-pointer to TcpServer
    m_battery_level(100),
    m_button_state(ButtonState::IDLE),
    m_button_armed_time_us(0),
    m_last_draw_status(Drawing<MAX_DRAW_BUFFER_PIXELS>::DrawStatus::IDLE)
{
    g_app_instance = this;

    m_release_timer.context = this;
    m_battery_timer.context = this;
    
    btstack_run_loop_set_timer_handler(&m_release_timer, &MediaApplication::release_handler_forwarder);
    btstack_run_loop_set_timer_handler(&m_battery_timer, &MediaApplication::battery_timer_handler_forwarder);
    
    BtStackManager::getInstance().registerHandler(&m_media_controller);

    critical_section_init(&m_tile_queue_crit_sec);
    m_tcp_server.setReceiveCallback(on_frame_received_callback);
}

// --- Strongly-typed handler for Image Tiles ---
void MediaApplication::on_image_tile_received(const Protocol::ImageTileHeader& header, const uint8_t* pixel_data, size_t pixel_len) {
    uint32_t calculated_crc = calculate_crc32(pixel_data, pixel_len);

    if (calculated_crc == header.crc32) {
        printf("DEBUG: CRC OK. Pushing tile (%dx%d at %d,%d) to queue.\n", header.width, header.height, header.x, header.y);
        
        Protocol::Frame frame;
        frame.header.magic = Protocol::FRAME_MAGIC;
        frame.header.type = Protocol::FrameType::IMAGE_TILE;
        frame.header.payload_length = sizeof(header) + pixel_len;
        
        // Ensure we don't overflow the fixed-size payload buffer
        size_t payload_to_copy = std::min((size_t)frame.header.payload_length, frame.payload.size());

        memcpy(frame.payload.data(), &header, sizeof(header));
        memcpy(frame.payload.data() + sizeof(header), pixel_data, payload_to_copy - sizeof(header));
        
        push_tile_to_queue(frame);
    } else {
        printf("!!! CHECKSUM MISMATCH !!! Host: 0x%08lX, Pico: 0x%08lX\n", header.crc32, calculated_crc);
        m_tcp_server.send_frame(Protocol::FrameType::TILE_NACK, nullptr, 0);
    }
}

void MediaApplication::push_tile_to_queue(const Protocol::Frame& frame) {
    critical_section_enter_blocking(&m_tile_queue_crit_sec);
    if (m_queue_count < TILE_QUEUE_SIZE) {
        m_tile_queue[m_queue_head] = frame;
        m_queue_head = (m_queue_head + 1) % TILE_QUEUE_SIZE;
        m_queue_count++;
    } else {
        printf("WARN: Tile queue overflow, dropping tile!\n");
    }
    critical_section_exit(&m_tile_queue_crit_sec);
}

void MediaApplication::run() {
    printf("Initializing Rotary Encoder...\n");
    m_encoder.init();
    sleep_ms(50);
    m_encoder.read_and_clear_rotation();

    printf("Initializing Display...\n");
    m_display.init();
    m_display.fillScreen(0); // Black
    m_drawing.drawString(10, 10, "Connecting to Wi-Fi...", 0xFFFF, &font_freesans_16);
    
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi network: %s\n", WIFI_SSID);

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Failed to connect to Wi-Fi.\n");
        m_drawing.fillRect(10, 10, 300, 20, 0);
        m_drawing.drawString(10, 10, "Wi-Fi connection failed.", 0xF800, &font_freesans_16);
    } else {
        printf("Connected to Wi-Fi. IP: %s\n", ip4addr_ntoa(netif_ip4_addr(&cyw43_state.netif[0])));
        m_tcp_server.init(4242);
        m_drawing.fillRect(10, 10, 300, 20, 0);
        m_drawing.drawString(10, 10, "Waiting for host...", 0x07E0, &font_freesans_16);
    }
    
    printf("Initializing BTstack...\n");
    m_media_controller.setup();
    m_media_controller.powerOn();
    
    btstack_run_loop_set_timer(&m_battery_timer, 30000);
    btstack_run_loop_add_timer(&m_battery_timer);

    // ---  MAIN LOOP ---
    while (true) {
        // This is the most important call. It handles all background work for
        // BTstack and lwIP, and then sleeps for a short time.
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(10));

        // --- Now, perform all our application logic sequentially ---

        // 1. Handle Encoder (always runs, non-blocking)
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
        
        // 2. Drive the non-blocking drawing engine and TCP logic
        auto current_draw_status = m_drawing.processDrawing();

        if (m_last_draw_status == Drawing<MAX_DRAW_BUFFER_PIXELS>::DrawStatus::BUSY && current_draw_status == Drawing<MAX_DRAW_BUFFER_PIXELS>::DrawStatus::IDLE) {
            printf("DEBUG: Drawing finished. Sending ACK.\n");
            m_tcp_server.send_frame(Protocol::FrameType::TILE_ACK, nullptr, 0);
        } 
        else if (current_draw_status == Drawing<MAX_DRAW_BUFFER_PIXELS>::DrawStatus::IDLE) {
            bool has_new_tile = false;
            Protocol::Frame new_tile_to_draw;

            critical_section_enter_blocking(&m_tile_queue_crit_sec);
            if (m_queue_count > 0) {
                has_new_tile = true;
                new_tile_to_draw = m_tile_queue[m_queue_tail];
                m_queue_tail = (m_queue_tail + 1) % TILE_QUEUE_SIZE;
                m_queue_count--;
            }
            critical_section_exit(&m_tile_queue_crit_sec);

            if (has_new_tile) {
                printf("DEBUG: Dequeued tile. Starting async draw.\n");
                Protocol::ImageTileHeader tile_header;
                memcpy(&tile_header, new_tile_to_draw.payload.data(), sizeof(tile_header));
                
                const uint16_t* pixel_data = reinterpret_cast<const uint16_t*>(
                    new_tile_to_draw.payload.data() + sizeof(tile_header)
                );
                
                m_drawing.drawImageAsync(tile_header.x, tile_header.y, tile_header.width, tile_header.height, pixel_data);
            }
        }
        m_last_draw_status = current_draw_status;
    }
}

// --- Static Forwarders ---
void MediaApplication::release_handler_forwarder(btstack_timer_source_t* ts) { static_cast<MediaApplication*>(ts->context)->release_handler(); }
void MediaApplication::battery_timer_handler_forwarder(btstack_timer_source_t* ts) { static_cast<MediaApplication*>(ts->context)->battery_timer_handler(); }

// --- Member Handlers ---

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
