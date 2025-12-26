// File: include/media/MediaControllerDevice.h

#ifndef MEDIA_CONTROLLER_DEVICE_H
#define MEDIA_CONTROLLER_DEVICE_H

#include "HidDevice.h"
#include <cstdint>

class MediaControllerDevice : public HidDevice {
public:
    void setup() override;
    void enterSetupMode();
    void setBatteryLevel(uint8_t level);

    // Media Control Methods
    void increaseVolume();
    void decreaseVolume();
    void mute();
    void playPause();
    void nextTrack();
    void previousTrack();
    void release();

protected:
    // Implementations for the pure virtual functions from HidDevice
    const uint8_t* getHidDescriptor() const override;
    uint16_t getHidDescriptorSize() const override;
    const uint8_t* getAdvertisingData() const override;
    uint16_t getAdvertisingDataSize() const override;

    bool m_setup_mode = false;
};

#endif // MEDIA_CONTROLLER_DEVICE_H
