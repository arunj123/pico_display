#ifndef MEDIA_CONTROLLER_DEVICE_H
#define MEDIA_CONTROLLER_DEVICE_H

#include "HidDevice.h"

class MediaControllerDevice : public HidDevice {
public:
    // --- FIX: Add the missing override declaration for setup() ---
    void setup() override;

    void increaseVolume();
    void decreaseVolume();
    void release(); // Release all keys

protected:
    const uint8_t* getHidDescriptor() const override;
    uint16_t getHidDescriptorSize() const override;
    const uint8_t* getAdvertisingData() const override;
    uint16_t getAdvertisingDataSize() const override;
};

#endif // MEDIA_CONTROLLER_DEVICE_H