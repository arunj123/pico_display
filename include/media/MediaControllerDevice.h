// File: include/media/MediaControllerDevice.h

#ifndef MEDIA_CONTROLLER_DEVICE_H
#define MEDIA_CONTROLLER_DEVICE_H

#include "HidDevice.h"
#include "FileTransferService.h"
#include <vector>
#include <cstdint>

class MediaControllerDevice : public HidDevice {
public:
    void setup() override;
    void setBatteryLevel(uint8_t level);

    bool startSendingData(const std::vector<uint8_t>& data, FileTransfer::DataType type);
    void setReceiveCallback(void (*callback)(FileTransfer::DataType, const std::vector<uint8_t>&));

    // Media Control Methods
    void increaseVolume();
    void decreaseVolume();
    void mute();
    void playPause();
    void nextTrack();
    void previousTrack();
    void release();

    // --- Publicly accessible for the static C callback ---
    int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

protected:
    // Implementations for the pure virtual functions from HidDevice
    const uint8_t* getHidDescriptor() const override;
    uint16_t getHidDescriptorSize() const override;
    const uint8_t* getAdvertisingData() const override;
    uint16_t getAdvertisingDataSize() const override;
    
    void onReadyToSend_impl() override;

private:
    enum class TransferState { IDLE, RECEIVING, SENDING };
    TransferState m_transfer_state = TransferState::IDLE;
    std::vector<uint8_t> m_rx_buffer;
    uint32_t m_rx_expected_size = 0;
    FileTransfer::DataType m_rx_type;
    void (*m_rx_callback)(FileTransfer::DataType, const std::vector<uint8_t>&) = nullptr;
    std::vector<uint8_t> m_tx_buffer;
    FileTransfer::DataType m_tx_type;
    uint32_t m_tx_offset = 0;
};

#endif
