// File: MediaControllerDevice.h

#ifndef MEDIA_CONTROLLER_DEVICE_H
#define MEDIA_CONTROLLER_DEVICE_H

#include "HidDevice.h"

class MediaControllerDevice : public HidDevice {
public:
    void setup() override;
    void setBatteryLevel(uint8_t level);

    // --- Media Control Methods ---
    void increaseVolume();
    void decreaseVolume();
    void mute();
    void playPause();
    void nextTrack();
    void previousTrack();
    void release(); // Release all keys

protected:
    const uint8_t* getHidDescriptor() const override;
    uint16_t getHidDescriptorSize() const override;
    const uint8_t* getAdvertisingData() const override;
    uint16_t getAdvertisingDataSize() const override;

private:
};

#endif // MEDIA_CONTROLLER_DEVICE_H