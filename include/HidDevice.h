#ifndef HID_DEVICE_H
#define HID_DEVICE_H

#include "I_BtStackHandler.h"

class HidDevice : public I_BtStackHandler {
public:
    HidDevice();
    
    virtual void setup();
    void powerOn();
    bool isConnected() const;
    void requestToSend();

    void disconnect();

    // I_BtStackHandler implementation
    void handlePacket(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) override;
    void onReadyToSend() override;
    void onHidSubscribed() override;
    void typingTimerHandler(btstack_timer_source_t* ts) override;

protected:
    // Virtual hooks for derived classes
    virtual void onReadyToSend_impl();
    virtual void onHidSubscribed_impl();
    virtual void onTypingTimer_impl(btstack_timer_source_t* ts);

    virtual const uint8_t* getHidDescriptor() const = 0;
    virtual uint16_t getHidDescriptorSize() const = 0;
    virtual const uint8_t* getAdvertisingData() const = 0;
    virtual uint16_t getAdvertisingDataSize() const = 0;
    
    void sendHidReport(const uint8_t* report, uint16_t size);

    hci_con_handle_t m_connection_handle;
};

#endif // HID_DEVICE_H