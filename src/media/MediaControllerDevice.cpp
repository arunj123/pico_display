// File: src/media/MediaControllerDevice.cpp

#include "MediaControllerDevice.h"
#include "media_controller.h"
#include "BleDescriptors.h"
#include "ble/gatt-service/battery_service_server.h"
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

void MediaControllerDevice::setup() {
    HidDevice::setup();
    att_server_init(profile_data, nullptr, nullptr);
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