#ifndef I_BT_STACK_HANDLER_H
#define I_BT_STACK_HANDLER_H

#include <cstdint>
#include "btstack.h"

class I_BtStackHandler {
public:
    virtual ~I_BtStackHandler() = default;
    virtual void handlePacket(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) = 0;
    virtual void onReadyToSend() = 0;
    virtual void onHidSubscribed() = 0;
    virtual void typingTimerHandler(btstack_timer_source_t* ts) = 0;
};

#endif // I_BT_STACK_HANDLER_H