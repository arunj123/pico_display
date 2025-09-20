#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include "btstack.h"
#include "KeyboardLayout.h" // Include the new layout header
#include <cstdint>
// <memory> is no longer needed

class HidKeyboard {
public:
    HidKeyboard();
    // --- FIX: Destructor is no longer needed for a direct member variable ---
    // ~HidKeyboard(); 

    void setup();
    void powerOn();

    void handlePacket(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    void canSendNow();
    void processStdin(char character);

private:
    enum class ProtocolMode : uint8_t {
        Boot = 0,
        Report = 1
    };

    enum class TypingState {
        Idle,
        WaitingToSendFromBuffer,
        WaitingToSendKeyUp
    };

    static constexpr uint8_t BATTERY_LEVEL = 100;
    static constexpr uint16_t ADV_INTERVAL_MIN = 0x0030;
    static constexpr uint16_t ADV_INTERVAL_MAX = 0x0030;

    void setupServices();
    void setupAdvertisements();
    void sendReport(uint8_t modifier, uint8_t keycode);
    
    btstack_packet_callback_registration_t m_hci_event_callback_reg;
    btstack_packet_callback_registration_t m_sm_event_callback_reg;
    hci_con_handle_t m_connection_handle;
    ProtocolMode m_protocol_mode;
    
    // --- FIX: Use a direct member variable for 100% static allocation ---
    USKeyboardLayout m_layout;

#ifdef HAVE_BTSTACK_STDIN
    TypingState m_typing_state;
    btstack_ring_buffer_t m_ascii_input_buffer;
    uint8_t m_ascii_input_storage[20];
#else
    void startTypingDemo();
    void typingTimerHandler(btstack_timer_source_t* ts);
    static void s_typingTimerHandler(btstack_timer_source_t* ts);

    static constexpr int TYPING_PERIOD_MS = 50;
    const char* m_demo_text = "\n\nHello World!\n\nThis is a C++ BTstack HID Keyboard.\n\n";
    int m_demo_pos;
    btstack_timer_source_t m_typing_timer;
    Keypress m_key_to_send;
    bool m_send_keyup;
#endif
};

#endif // HID_KEYBOARD_H