#pragma once

#include "Drawing.h"
#include "HidDevice.h"
#include "SetupDrawing.h"

class MediaControllerDevice : public HidDevice {
public:
    // Update constructor to accept Drawing reference
    MediaControllerDevice(Drawing& drawing);

    void setup() override;
    void handlePacket(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) override;
    
    // Public Accessor (Required by MediaApplication.cpp)
    bool isSetupMode() const { return m_setup_mode; }

    void enterSetupMode();
    void forceExitSetupMode(); // Public method for button exit
    
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
    const uint8_t* getHidDescriptor() const override;
    uint16_t getHidDescriptorSize() const override;
    const uint8_t* getAdvertisingData() const override;
    uint16_t getAdvertisingDataSize() const override;

private:
    bool m_setup_mode = false;
    
    // Drawing components
    SetupDrawing m_setupDrawing; 
};