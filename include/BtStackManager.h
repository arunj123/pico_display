#ifndef BT_STACK_MANAGER_H
#define BT_STACK_MANAGER_H

#include "pico/stdlib.h"
#include "btstack.h"
#include "I_BtStackHandler.h" // <-- FIX: Use the new interface filename

class BtStackManager {
public:
    static BtStackManager& getInstance();
    
    // --- FIX: Update the handler type to the new interface name ---
    void registerHandler(I_BtStackHandler* handler);

    static void typingTimerForwarder(btstack_timer_source_t* ts);

private:
    BtStackManager();
    BtStackManager(const BtStackManager&) = delete;
    void operator=(const BtStackManager&) = delete;

    static void packetHandlerForwarder(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    static void canSendNowForwarder();
    
    btstack_packet_callback_registration_t m_hci_event_callback_reg;
    btstack_packet_callback_registration_t m_sm_event_callback_reg;

    I_BtStackHandler* m_handler;
};

#endif // BT_STACK_MANAGER_H