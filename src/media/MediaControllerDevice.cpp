// File: src/media/MediaControllerDevice.cpp

#include "MediaControllerDevice.h"
#include "MediaApplication.h"
#include "media_controller.h"
#include "BleDescriptors.h"
#include "ble/gatt-service/battery_service_server.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

extern MediaApplication* g_app_instance;

// --- Define bitmasks for each media key ---
namespace {
    // Bitmasks...
    constexpr uint8_t REPORT_MASK_VOLUME_UP   = 1 << 0;
    constexpr uint8_t REPORT_MASK_VOLUME_DOWN = 1 << 1;
    constexpr uint8_t REPORT_MASK_MUTE        = 1 << 2;
    constexpr uint8_t REPORT_MASK_PLAY_PAUSE  = 1 << 3;
    constexpr uint8_t REPORT_MASK_NEXT_TRACK  = 1 << 4;
    constexpr uint8_t REPORT_MASK_PREV_TRACK  = 1 << 5;
}

// --- Static C-style callback that forwards to the C++ class instance ---
static int att_write_callback_forwarder(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    if (g_app_instance) {
        return g_app_instance->getMediaController().att_write_callback(con_handle, att_handle, transaction_mode, offset, buffer, buffer_size);
    }
    return 0;
}

void MediaControllerDevice::setup() {
    // 1. Call the base class's generic setup
    HidDevice::setup();
    
    // 2. Perform media-controller-specific setup    
    // --- Register the write callback during server initialization ---
    att_server_init(profile_data, nullptr, &att_write_callback_forwarder);
    
    hids_device_init(0, getHidDescriptor(), getHidDescriptorSize());
    battery_service_server_init(100);
    device_information_service_server_init();
    device_information_service_server_set_manufacturer_name("Pico Projects");
    device_information_service_server_set_model_number("PIO Encoder v1.0");
    device_information_service_server_set_serial_number("12345678");

    bd_addr_t null_addr = {0};
    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(getAdvertisingDataSize(), (uint8_t*)getAdvertisingData());
    gap_advertisements_enable(1);
}

// --- The instance method that handles the write, now matching the header ---
int MediaControllerDevice::att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    if (att_handle == ATT_CHARACTERISTIC_0000FF53_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) { // Control Point
        if (buffer_size < 1) return 0;
        auto cmd = (FileTransfer::Command)buffer[0];

        if (cmd == FileTransfer::Command::START_TRANSFER) {
            if (buffer_size != 6) return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
            if (m_transfer_state != TransferState::IDLE) return ATT_ERROR_REQUEST_NOT_SUPPORTED;
            m_rx_expected_size = little_endian_read_32(buffer, 1);
            m_rx_type = (FileTransfer::DataType)buffer[5];
            if (m_rx_expected_size == 0 || m_rx_expected_size > 8192) return ATT_ERROR_INSUFFICIENT_RESOURCES;
            printf("Starting RX: %lu bytes, type %d\n", m_rx_expected_size, (int)m_rx_type);
            m_rx_buffer.clear();
            m_rx_buffer.reserve(m_rx_expected_size);
            m_transfer_state = TransferState::RECEIVING;
        }

    } else if (att_handle == ATT_CHARACTERISTIC_0000FF52_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) { // RX Data
        if (m_transfer_state != TransferState::RECEIVING) return ATT_ERROR_REQUEST_NOT_SUPPORTED;
        m_rx_buffer.insert(m_rx_buffer.end(), buffer, buffer + buffer_size);
        printf("Received chunk: %d bytes (Total: %zu/%lu)\n", buffer_size, m_rx_buffer.size(), m_rx_expected_size);
        if (m_rx_buffer.size() >= m_rx_expected_size) {
            printf("RX complete.\n");
            if (m_rx_callback) m_rx_callback(m_rx_type, m_rx_buffer);
            m_transfer_state = TransferState::IDLE;
        }
    }
    return 0; // Success
}

void MediaControllerDevice::setBatteryLevel(uint8_t level) {
    battery_service_server_set_battery_value(level);
}

bool MediaControllerDevice::startSendingData(const std::vector<uint8_t>& data, FileTransfer::DataType type) {
    if (m_transfer_state != TransferState::IDLE || !isConnected()) return false;
    m_transfer_state = TransferState::SENDING;
    m_tx_buffer = data;
    m_tx_type = type;
    m_tx_offset = 0;

    uint8_t start_cmd[6];
    start_cmd[0] = (uint8_t)FileTransfer::Command::START_TRANSFER;
    little_endian_store_32(start_cmd, 1, data.size());
    start_cmd[5] = (uint8_t)type;

    printf("Starting TX: %zu bytes, type %d\n", data.size(), (int)type);
    att_server_notify(m_connection_handle, ATT_CHARACTERISTIC_0000FF53_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, start_cmd, sizeof(start_cmd));
    requestToSend();
    return true;
}

void MediaControllerDevice::onReadyToSend_impl() {
    if (m_transfer_state != TransferState::SENDING) return;
    if (m_tx_offset >= m_tx_buffer.size()) {
        printf("TX complete.\n");
        m_transfer_state = TransferState::IDLE;
        return;
    }
    uint16_t mtu = l2cap_get_remote_mtu_for_local_cid(m_connection_handle);
    uint16_t chunk_size = std::min((size_t)(mtu - 3), (size_t)(m_tx_buffer.size() - m_tx_offset));
    att_server_notify(m_connection_handle, ATT_CHARACTERISTIC_0000FF51_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, &m_tx_buffer[m_tx_offset], chunk_size);
    m_tx_offset += chunk_size;
    requestToSend();
}

// --- Implementation of pure virtuals from HidDevice ---
const uint8_t* MediaControllerDevice::getHidDescriptor() const { return BleDescriptors::Media::hid_descriptor; }
uint16_t MediaControllerDevice::getHidDescriptorSize() const { return sizeof(BleDescriptors::Media::hid_descriptor); }
const uint8_t* MediaControllerDevice::getAdvertisingData() const { return BleDescriptors::Media::advertising_data; }
uint16_t MediaControllerDevice::getAdvertisingDataSize() const { return sizeof(BleDescriptors::Media::advertising_data); }

// --- Media Control Methods ---
void MediaControllerDevice::increaseVolume() {
    uint8_t report[] = {REPORT_MASK_VOLUME_UP};
    sendHidReport(report, sizeof(report));
}

void MediaControllerDevice::decreaseVolume() {
    uint8_t report[] = {REPORT_MASK_VOLUME_DOWN};
    sendHidReport(report, sizeof(report));
}

void MediaControllerDevice::mute() {
    uint8_t report[] = {REPORT_MASK_MUTE};
    sendHidReport(report, sizeof(report));
}

void MediaControllerDevice::playPause() {
    uint8_t report[] = {REPORT_MASK_PLAY_PAUSE};
    sendHidReport(report, sizeof(report));
}

void MediaControllerDevice::nextTrack() {
    uint8_t report[] = {REPORT_MASK_NEXT_TRACK};
    sendHidReport(report, sizeof(report));
}

void MediaControllerDevice::previousTrack() {
    uint8_t report[] = {REPORT_MASK_PREV_TRACK};
    sendHidReport(report, sizeof(report));
}

void MediaControllerDevice::release() {
    uint8_t report[] = {0x00}; // All bits zero
    sendHidReport(report, sizeof(report));
}