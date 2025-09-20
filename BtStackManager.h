#ifndef BT_STACK_MANAGER_H
#define BT_STACK_MANAGER_H

#include "pico/stdlib.h"
#include "btstack.h"
#include "I_HidKeyboardHandler.h"

// Manages the interface between the C-based BTstack and C++ handlers
class BtStackManager {
public:
    // Singleton access
    static BtStackManager& getInstance();

    // Register the object that will handle the events
    void registerHandler(I_HidKeyboardHandler* handler);

    // Public method to be called by the timer handler in the demo mode
    static void typingTimerForwarder(btstack_timer_source_t* ts);

private:
    // Private constructor for singleton
    BtStackManager();

    // Delete copy and assignment operators
    BtStackManager(const BtStackManager&) = delete;
    void operator=(const BtStackManager&) = delete;

    // C-style callbacks that forward to the registered handler
    static void packetHandlerForwarder(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    static void canSendNowForwarder();

    btstack_packet_callback_registration_t m_hci_event_callback_reg;
    btstack_packet_callback_registration_t m_sm_event_callback_reg;

    // The handler to forward events to
    I_HidKeyboardHandler* m_handler;
};

#endif // BT_STACK_MANAGER_H