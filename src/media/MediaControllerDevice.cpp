// File: src/media/MediaControllerDevice.cpp

#include "MediaControllerDevice.h"
#include "hardware/watchdog.h"
#include "WifiConfig.h"
#include "media_controller.h"
#include "BleDescriptors.h"
#include "ble/gatt-service/battery_service_server.h"
#include "pico/stdlib.h" // For sleep_ms

#include <string>
#include <cstring>
#include <cstdio>

namespace {
    // Bitmasks for HID reports
    constexpr uint8_t REPORT_MASK_VOLUME_UP   = 1 << 0;
    constexpr uint8_t REPORT_MASK_VOLUME_DOWN = 1 << 1;
    constexpr uint8_t REPORT_MASK_MUTE        = 1 << 2;
    constexpr uint8_t REPORT_MASK_PLAY_PAUSE  = 1 << 3;
    constexpr uint8_t REPORT_MASK_NEXT_TRACK  = 1 << 4;
    constexpr uint8_t REPORT_MASK_PREV_TRACK  = 1 << 5;
}

// Define a simpler advertising packet for Setup Mode (No HID UUIDs)
const uint8_t setup_adv_data[] = {
    // Flags: General Discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // Name: Pico Setup
    0x0B, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', 'S', 'e', 't', 'u', 'p',
    // Incomplete List of 128-bit Service UUIDs (The Config Service)
    0x11, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00
};

int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    
    // Debug Print
    printf("ATT Write: Handle=0x%04X, Len=%d, Mode=%d\n", att_handle, buffer_size, transaction_mode);

    if (att_handle == ATT_CHARACTERISTIC_0000FF01_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        std::string payload((char*)buffer, buffer_size);
        printf("Payload received: %s\n", payload.c_str()); // Print the received string

        size_t separator = payload.find(':');
        
        if (separator != std::string::npos) {
            std::string ssid = payload.substr(0, separator);
            std::string pass = payload.substr(separator + 1);
            
            WifiConfig::save(ssid.c_str(), pass.c_str());
            
            printf("Rebooting in 2 seconds...\n");
            stdio_flush();
            
            // --- IMPORTANT: Wait for print buffer to flush and flash to settle ---
            sleep_ms(2000); 
            watchdog_reboot(0, 0, 0);
        } else {
            printf("Error: Invalid payload format (Missing ':')\n");
        }
        return 0; 
    }
    return 0;
}

void MediaControllerDevice::setup() {
    HidDevice::setup();

    printf("Registering ATT Write Callback...\n");

    att_server_init(profile_data, nullptr, att_write_callback);
    hids_device_init(0, getHidDescriptor(), getHidDescriptorSize());
    battery_service_server_init(100);
    device_information_service_server_init();
    device_information_service_server_set_manufacturer_name("Pico Projects");
    device_information_service_server_set_model_number("PIO Encoder v1.0");

    bd_addr_t null_addr = {0};
    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(getAdvertisingDataSize(), (uint8_t*)getAdvertisingData());
    gap_advertisements_enable(1);
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

void MediaControllerDevice::enterSetupMode() {
    printf("Entering Setup Mode (HID Disabled)\n");
    m_setup_mode = true;
    
    // 1. Kill existing connection (Kill the Zombie)
    disconnect();
    
    // 2. Stop Advertising
    gap_advertisements_enable(0);
    sleep_ms(50); // Give the stack a moment
    
    // 3. --- NEW: Change Identity to Random Address ---
    // This fools Windows into thinking we are a stranger.
    bd_addr_t random_addr;
    // We'll generate a static random address: 0xC0 (top bits 11) ensures it's "Random Static"
    // We can just use a fixed one for setup mode: C0:FF:EE:C0:FF:EE
    bd_addr_t setup_addr = {0xC0, 0xFF, 0xEE, 0xC0, 0xFF, 0xEE};
    
    // Configure stack to use this random address
    gap_random_address_set(setup_addr);
    
    // Tell advertising to USE the random address (0x01 = Random Address Type)
    // Params: min, max, adv_type, direct_addr_type, direct_addr, channel_map, filter_policy
    // Note: BTstack usually sets the own_addr_type via gap_advertisements_set_params logic
    // but we might need to rely on the default behavior picking up the set random address 
    // if we don't explicitly control the "own_address_type" parameter (which is hidden in some API wrappers).
    // However, on Pico SDK's BTstack, simply setting the random address is often enough if the Advertising parameters are refreshed.
    
    // Let's force update parameters. 
    // We keep default intervals (0x0030) but we need to ensure the stack knows we swapped addresses.
    // NOTE: The Pico-SDK version of gap_advertisements_set_params DOES NOT take "own_address_type".
    // It is inferred. We must hope gap_random_address_set takes precedence when configured.
    // ---------------------------------------------------

    // 4. Set the new name "Pico Setup"
    gap_advertisements_set_data(sizeof(setup_adv_data), (uint8_t*)setup_adv_data);
    
    // 5. Restart Advertising
    gap_advertisements_enable(1);
    
    printf("Now advertising as 'Pico Setup' (Address Spoofed).\n");
}