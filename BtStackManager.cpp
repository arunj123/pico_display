#include "BtStackManager.h"
#include <cstring>

// --- Singleton Implementation ---

// Meyers' Singleton: thread-safe initialization guaranteed by the language
BtStackManager& BtStackManager::getInstance() {
    static BtStackManager instance;
    return instance;
}

// Global pointer to the singleton instance for C callbacks
static BtStackManager* g_instance = nullptr;

BtStackManager::BtStackManager() : m_handler(nullptr) {
    g_instance = this;
    memset(&m_hci_event_callback_reg, 0, sizeof(m_hci_event_callback_reg));
    memset(&m_sm_event_callback_reg, 0, sizeof(m_sm_event_callback_reg));
}

void BtStackManager::registerHandler(I_HidKeyboardHandler* handler) {
    m_handler = handler;

    // Now that we have a handler, we can register the callbacks with BTstack
    m_hci_event_callback_reg.callback = &BtStackManager::packetHandlerForwarder;
    hci_add_event_handler(&m_hci_event_callback_reg);

    m_sm_event_callback_reg.callback = &BtStackManager::packetHandlerForwarder;
    sm_add_event_handler(&m_sm_event_callback_reg);

    hids_device_register_packet_handler(&BtStackManager::packetHandlerForwarder);
}

// --- C-style Callback Forwarders ---

void BtStackManager::packetHandlerForwarder(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (g_instance && g_instance->m_handler) {
        g_instance->m_handler->handlePacket(packet_type, channel, packet, size);
    }
}

void BtStackManager::canSendNowForwarder() {
    if (g_instance && g_instance->m_handler) {
        g_instance->m_handler->canSendNow();
    }
}

void BtStackManager::typingTimerForwarder(btstack_timer_source_t* ts) {
#ifndef HAVE_BTSTACK_STDIN
    if (g_instance && g_instance->m_handler) {
        g_instance->m_handler->typingTimerHandler(ts);
    }
#endif
}