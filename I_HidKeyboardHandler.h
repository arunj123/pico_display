#ifndef I_HID_KEYBOARD_HANDLER_H
#define I_HID_KEYBOARD_HANDLER_H

#include <cstdint>
#include "btstack.h"

// Interface class for handling events forwarded from the BtStackManager
class I_HidKeyboardHandler {
public:
    virtual ~I_HidKeyboardHandler() = default;

    // Called when a BTstack packet arrives
    virtual void handlePacket(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) = 0;
    
    // Called when the BTstack is ready to send a report
    virtual void canSendNow() = 0;

#ifndef HAVE_BTSTACK_STDIN
    // Called by the timer to send the next character in the demo
    virtual void typingTimerHandler(btstack_timer_source_t* ts) = 0;
#endif
};

#endif // I_HID_KEYBOARD_HANDLER_H