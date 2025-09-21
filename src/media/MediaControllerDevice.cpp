#include "MediaControllerDevice.h"
#include "media_controller.h"
#include "BleDescriptors.h"

// --- Implementation of HidDevice virtual methods ---

void MediaControllerDevice::setup() {
    // 1. Call the base class's generic setup
    HidDevice::setup();

    // 2. Perform media-controller-specific setup
    // This profile_data now comes from media_controller.h
    att_server_init(profile_data, nullptr, nullptr); 
    hids_device_init(0, getHidDescriptor(), getHidDescriptorSize());
    battery_service_server_init(100);
    device_information_service_server_init();

    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(getAdvertisingDataSize(), (uint8_t*)getAdvertisingData());
    gap_advertisements_enable(1);
}

const uint8_t* MediaControllerDevice::getHidDescriptor() const {
    return BleDescriptors::Media::hid_descriptor;
}
uint16_t MediaControllerDevice::getHidDescriptorSize() const {
    return sizeof(BleDescriptors::Media::hid_descriptor);
}
const uint8_t* MediaControllerDevice::getAdvertisingData() const {
    return BleDescriptors::Media::advertising_data;
}
uint16_t MediaControllerDevice::getAdvertisingDataSize() const {
    return sizeof(BleDescriptors::Media::advertising_data);
}

// --- Media Control Methods ---

void MediaControllerDevice::increaseVolume() {
    uint8_t report[] = {0x01}; // Bit 0 for Volume Increment
    sendHidReport(report, sizeof(report));
}

void MediaControllerDevice::decreaseVolume() {
    uint8_t report[] = {0x02}; // Bit 1 for Volume Decrement
    sendHidReport(report, sizeof(report));
}

void MediaControllerDevice::release() {
    uint8_t report[] = {0x00}; // All bits zero
    sendHidReport(report, sizeof(report));
}