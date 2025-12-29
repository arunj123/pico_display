// File: src/media/MediaControllerDevice.cpp

#include "MediaControllerDevice.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h" 
#include "WifiConfig.h"
#include "media_controller.h" 
#include "BleDescriptors.h"
#include "ble/gatt-service/battery_service_server.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/unique_id.h" 
#include "btstack_run_loop.h"
#include "btstack_event.h"
#include "btstack_crypto.h" 

#include <string>
#include <cstring>
#include <cstdio>

extern "C" const uint8_t * get_setup_profile_data(void);
extern "C" uint16_t get_setup_wifi_handle(void);

#define SETUP_MODE_MAGIC 0x7E57CAFE
#define BOOT_FLAG_REGISTER (watchdog_hw->scratch[7])

// Define the Setup URL for easy access
static const char* SETUP_WEB_URL = "https://arunj123.github.io/pico_display/web/";

// Timeout configuration
static const uint32_t SETUP_TIMEOUT_MS = 180000; // 3 Minutes
static absolute_time_t m_last_activity_time;

static uint16_t active_wifi_handle = 0;

const uint8_t setup_adv_data[] = {
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    0x0B, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', 'S', 'e', 't', 'u', 'p',
};

static std::string temp_ssid = "";
static std::string temp_pass = "";
static btstack_timer_source_t setup_poll_timer;

// Reboot scheduling to allow BLE Write Response to flush
static bool m_reboot_scheduled = false;
static absolute_time_t m_reboot_deadline;

namespace {
    constexpr uint8_t REPORT_MASK_VOLUME_UP     = 1 << 0;
    constexpr uint8_t REPORT_MASK_VOLUME_DOWN   = 1 << 1;
    constexpr uint8_t REPORT_MASK_MUTE          = 1 << 2;
    constexpr uint8_t REPORT_MASK_PLAY_PAUSE    = 1 << 3;
    constexpr uint8_t REPORT_MASK_NEXT_TRACK    = 1 << 4;
    constexpr uint8_t REPORT_MASK_PREV_TRACK    = 1 << 5;
}

// Constructor Implementation - Updated to use SetupDrawing
MediaControllerDevice::MediaControllerDevice(Drawing& drawing) 
    : m_setupDrawing(drawing) 
{
}

static void setup_poll_handler(btstack_timer_source_t *ts) {
    cyw43_arch_poll(); 

    // 1. Check for Reboot Schedule (Save/Exit Success)
    if (m_reboot_scheduled) {
        if (absolute_time_diff_us(get_absolute_time(), m_reboot_deadline) < 0) {
            printf("[BLE] Reboot scheduled. Resetting system now...\n");
            stdio_flush();
            watchdog_enable(1, 0);
            while(1);
        }
    } 
    // 2. Check for Inactivity Timeout (Only if reboot isn't already scheduled)
    else if (absolute_time_diff_us(m_last_activity_time, get_absolute_time()) > (SETUP_TIMEOUT_MS * 1000LL)) {
        printf("[BLE] Setup Timeout (%d sec inactivity). Exiting Setup Mode...\n", SETUP_TIMEOUT_MS / 1000);
        stdio_flush();
        BOOT_FLAG_REGISTER = 0; // Clear setup flag to boot normally next time
        watchdog_enable(1, 0);
        while(1);
    }

    btstack_run_loop_set_timer(ts, 5); 
    btstack_run_loop_add_timer(ts);
}

static void start_setup_advertising() {
    printf("[BLE] Stack Ready. Configuring Advertising...\n");
    stdio_flush();

    bd_addr_t setup_addr;
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);

    // Copy the last 6 bytes of the board ID (8 bytes total) to the setup address
    memcpy(setup_addr, &board_id.id[2], 6);
    
    // Force the top two bits to be 11 (Static Random Address requirement)
    setup_addr[0] |= 0xC0; 
    
    gap_random_address_set(setup_addr);

    bd_addr_t null_addr = {0};
    gap_advertisements_set_params(0x0030, 0x0030, 0, 1, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(setup_adv_data), (uint8_t*)setup_adv_data);
    gap_advertisements_enable(1);
    
    printf("[BLE] Advertising as 'Pico Setup' (Addr: %02X:%02X:%02X:%02X:%02X:%02X)\n",
        setup_addr[0], setup_addr[1], setup_addr[2], setup_addr[3], setup_addr[4], setup_addr[5]);
    stdio_flush();
}

int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(con_handle);
    UNUSED(transaction_mode);
    UNUSED(offset);

    // Reset activity timer on any write command
    m_last_activity_time = get_absolute_time();

    if (att_handle == 0 || buffer == nullptr || buffer_size == 0) return 0;

    printf("[BLE] Write: Handle=0x%04X, Len=%d\n", att_handle, buffer_size);
    stdio_flush();

    if (att_handle == active_wifi_handle) {
        std::string payload((char*)buffer, buffer_size);

        if (payload.rfind("S:", 0) == 0) { 
            temp_ssid = payload.substr(2);
            printf("[BLE] Buffered SSID: %s\n", temp_ssid.c_str());
        } 
        else if (payload.rfind("P:", 0) == 0) { 
            temp_pass = payload.substr(2);
            printf("[BLE] Buffered PASS: [Hidden]\n");
        } 
        else if (payload == "SAVE") {
            if (!temp_ssid.empty()) {
                printf("[BLE] Valid SSID received: %s\n", temp_ssid.c_str());
                printf("[BLE] Saving config to Flash...\n");
                
                WifiConfig::save(temp_ssid.c_str(), temp_pass.c_str());
                BOOT_FLAG_REGISTER = 0;
                
                printf("[BLE] Save Complete. Scheduling reboot in 1500ms...\n");
                m_reboot_deadline = make_timeout_time_ms(1500);
                m_reboot_scheduled = true;
                
            } else {
                printf("[BLE] Error: Cannot save empty SSID\n");
            }
        }
        else if (payload == "EXIT") {
            printf("[BLE] Exit command received. Rebooting without saving...\n");
            BOOT_FLAG_REGISTER = 0; // Clear setup flag
            
            // Schedule reboot to allow Write Response to go back to Web App
            m_reboot_deadline = make_timeout_time_ms(1500);
            m_reboot_scheduled = true;
        }
        return 0; 
    }
    return 0;
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
    UNUSED(connection_handle);
    UNUSED(att_handle);
    UNUSED(offset);
    UNUSED(buffer);
    UNUSED(buffer_size);
    return 0;
}

void MediaControllerDevice::setup() {
    HidDevice::setup();

    bool is_setup_mode = (BOOT_FLAG_REGISTER == SETUP_MODE_MAGIC);

    if (is_setup_mode) {
        m_setup_mode = true;
        m_last_activity_time = get_absolute_time(); // Initialize timeout timer
        BOOT_FLAG_REGISTER = 0;

        printf("\n!!! BOOTING IN SETUP MODE (Profile: setup_mode.gatt) !!!\n");
        printf("┌──────────────────────────────────────────────────────────────────┐\n");
        printf("│                       PICO SETUP REQUIRED                        │\n");
        printf("│                                                                  │\n");
        printf("│   1. Open URL to configure Wi-Fi:                                │\n");
        printf("│      %s   │\n", SETUP_WEB_URL);
        printf("│                                                                  │\n");
        printf("│   2. Connect to Bluetooth Device: 'Pico Setup'                   │\n");
        printf("│                                                                  │\n");
        printf("│   * Setup mode will timeout in %d seconds if inactive.           │\n", SETUP_TIMEOUT_MS / 1000);
        printf("└──────────────────────────────────────────────────────────────────┘\n");
        
        // Security: BONDING ENABLED
        sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
        sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING); 

        att_server_init(get_setup_profile_data(), att_read_callback, att_write_callback);
        active_wifi_handle = get_setup_wifi_handle();
        printf("[BLE] Wi-Fi Config Handle: 0x%04X\n", active_wifi_handle);

        battery_service_server_init(100);
        device_information_service_server_init();

        printf("[BLE] Configured for Setup. Returning to App to draw UI...\n");
        
        // --- DRAW USING SetupDrawing CLASS ---
        m_setupDrawing.showSetupScreen(SETUP_WEB_URL);
        
        stdio_flush();
        
        setup_poll_timer.process = &setup_poll_handler;
        btstack_run_loop_set_timer(&setup_poll_timer, 5);
        btstack_run_loop_add_timer(&setup_poll_timer);

    } else {
        printf("\n--- Booting in Normal Media Mode (Profile: media_controller.gatt) ---\n");
        m_setup_mode = false;

        sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
        sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

        att_server_init(profile_data, nullptr, nullptr); 
        active_wifi_handle = 0; 
        
        hids_device_init(0, getHidDescriptor(), getHidDescriptorSize());
        battery_service_server_init(100);
        device_information_service_server_init();
        
        bd_addr_t null_addr = {0};
        gap_advertisements_set_params(0x0030, 0x0030, 0, 0, null_addr, 0x07, 0x00);
        gap_advertisements_set_data(getAdvertisingDataSize(), (uint8_t*)getAdvertisingData());
        gap_advertisements_enable(1);
    }
}

// --- Exit Button Handler ---
void MediaControllerDevice::forceExitSetupMode() {
    printf("[Setup] Force Exit triggered by Button. Rebooting...\n");
    BOOT_FLAG_REGISTER = 0; // Ensure setup flag is cleared
    watchdog_enable(1, 0);  // Trigger immediate reboot
    while(1);
}

void MediaControllerDevice::handlePacket(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
    HidDevice::handlePacket(packet_type, channel, packet, size);

    if (packet_type == HCI_EVENT_PACKET) {
        uint8_t event = hci_event_packet_get_type(packet);

        if (event == BTSTACK_EVENT_STATE) {
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                if (m_setup_mode) start_setup_advertising();
            }
        }
        else if (event == HCI_EVENT_LE_META) {
            if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                 uint16_t conn_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                 printf("[BLE] LE Connection Complete. Handle: 0x%04X\n", conn_handle);
                 
                 // Reset inactivity timer on connection
                 m_last_activity_time = get_absolute_time();
                 
                 // PROACTIVELY REQUEST SECURITY
                 if (m_setup_mode) {
                     printf("[BLE] Requesting Security (Pairing)...\n");
                     sm_send_security_request(conn_handle);
                 }
                 stdio_flush();
            }
        }
        else if (event == HCI_EVENT_DISCONNECTION_COMPLETE) {
             printf("[BLE] Disconnected.\n");
             if (m_setup_mode) gap_advertisements_enable(1);
        }
        else if (event == SM_EVENT_JUST_WORKS_REQUEST) {
            printf("[BLE] Just Works Requested -> Confirming.\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
        }
        else if (event == SM_EVENT_PAIRING_COMPLETE) {
            printf("[BLE] Pairing Complete. Status: 0x%02X\n", sm_event_pairing_complete_get_status(packet));
        }
    }
}

void MediaControllerDevice::enterSetupMode() {
    printf("[BLE] Requesting Setup Mode -> Rebooting...\n");
    BOOT_FLAG_REGISTER = SETUP_MODE_MAGIC;
    watchdog_enable(1, 0); 
    while(true);
}

void MediaControllerDevice::setBatteryLevel(uint8_t level) {
    battery_service_server_set_battery_value(level);
}

const uint8_t* MediaControllerDevice::getHidDescriptor() const { return BleDescriptors::Media::hid_descriptor; }
uint16_t MediaControllerDevice::getHidDescriptorSize() const { return sizeof(BleDescriptors::Media::hid_descriptor); }
const uint8_t* MediaControllerDevice::getAdvertisingData() const { return BleDescriptors::Media::advertising_data; }
uint16_t MediaControllerDevice::getAdvertisingDataSize() const { return sizeof(BleDescriptors::Media::advertising_data); }

void MediaControllerDevice::increaseVolume() { uint8_t report[] = {REPORT_MASK_VOLUME_UP}; sendHidReport(report, sizeof(report)); }
void MediaControllerDevice::decreaseVolume() { uint8_t report[] = {REPORT_MASK_VOLUME_DOWN}; sendHidReport(report, sizeof(report)); }
void MediaControllerDevice::mute() { uint8_t report[] = {REPORT_MASK_MUTE}; sendHidReport(report, sizeof(report)); }
void MediaControllerDevice::playPause() { uint8_t report[] = {REPORT_MASK_PLAY_PAUSE}; sendHidReport(report, sizeof(report)); }
void MediaControllerDevice::nextTrack() { uint8_t report[] = {REPORT_MASK_NEXT_TRACK}; sendHidReport(report, sizeof(report)); }
void MediaControllerDevice::previousTrack() { uint8_t report[] = {REPORT_MASK_PREV_TRACK}; sendHidReport(report, sizeof(report)); }
void MediaControllerDevice::release() { uint8_t report[] = {0x00}; sendHidReport(report, sizeof(report)); }