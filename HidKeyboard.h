#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include "I_HidKeyboardHandler.h"
#include "KeyboardLayout.h"
#include "HidReport.h" // <-- Include the new report header

// A clean C++ class for the HID Keyboard logic
template <typename LayoutType>
class HidKeyboard : public I_HidKeyboardHandler {
public:
    HidKeyboard();

    void setup();
    void powerOn();
    void processStdin(char character);
    
    // --- Implementation of the I_HidKeyboardHandler interface ---
    void handlePacket(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) override;
    void canSendNow() override;
#ifndef HAVE_BTSTACK_STDIN
    void typingTimerHandler(btstack_timer_source_t* ts) override;
#endif

private:
    enum class ProtocolMode : uint8_t { Boot = 0, Report = 1 };
    enum class TypingState { Idle, WaitingToSendFromBuffer, WaitingToSendKeyUp };

    static constexpr uint8_t BATTERY_LEVEL = 100;
    static constexpr uint16_t ADV_INTERVAL_MIN = 0x0030;
    static constexpr uint16_t ADV_INTERVAL_MAX = 0x0030;

    void setupServices();
    void setupAdvertisements();
    // --- FIX: Update sendReport to take the new struct ---
    void sendReport(const HidReport& report);
    
    hci_con_handle_t m_connection_handle;
    ProtocolMode m_protocol_mode;
    
    LayoutType m_layout;

#ifdef HAVE_BTSTACK_STDIN
    TypingState m_typing_state;
    btstack_ring_buffer_t m_ascii_input_buffer;
    uint8_t m_ascii_input_storage[20];
    HidReport m_report_to_send; // Keep track of the report to send
#else
    void startTypingDemo();

    static constexpr int TYPING_PERIOD_MS = 50;
    const char* m_demo_text = "\n\nHello World!\n\nThis is a C++ BTstack HID Keyboard.\n\n";
    int m_demo_pos = 0;
    btstack_timer_source_t m_typing_timer;
    HidReport m_report_to_send; // Use the struct instead of separate key/modifier
    bool m_send_keyup = false;
#endif
};

// Explicitly instantiate the template for the US Layout.
extern template class HidKeyboard<USKeyboardLayout>;

#endif // HID_KEYBOARD_H