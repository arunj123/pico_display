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

// Define a write callback
int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    printf("ATT Write: Handle=0x%04X, Len=%d\n", att_handle, buffer_size);

    // Check if the write is for our Wifi Characteristic
    if (att_handle == ATT_CHARACTERISTIC_0000FF01_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        
        // Parse "SSID:PASSWORD"
        std::string payload((char*)buffer, buffer_size);
        size_t separator = payload.find(':');
        
        if (separator != std::string::npos) {
            std::string ssid = payload.substr(0, separator);
            std::string pass = payload.substr(separator + 1);
            
            printf("Received Wi-Fi Config via BLE. Saving...\n");
            
            // Save to Flash
            WifiConfig::save(ssid.c_str(), pass.c_str());
            
            // Optional: Trigger a reboot or a state machine event to connect
            watchdog_reboot(0, 0, 100); 
        }
        return 0; // OK
    }
    return 0;
}

void MediaControllerDevice::setup() {
    HidDevice::setup();
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
    
    // 1. Try to disconnect if we think we are connected
    disconnect();
    
    // 2. Stop Advertising completely
    gap_advertisements_enable(0);
    
    // 3. Small blocking delay to ensure the stack processes the disconnect/stop
    // This is safe here because we are about to enter a configuration mode
    sleep_ms(50);
    
    // 4. Update the data to "Pico Setup"
    gap_advertisements_set_data(sizeof(setup_adv_data), (uint8_t*)setup_adv_data);
    
    // 5. Restart Advertising
    gap_advertisements_enable(1);
    
    printf("Now advertising as 'Pico Setup'. Please connect via Web Page.\n");
}