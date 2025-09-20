#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include "I_HidKeyboardHandler.h"
#include "KeyboardLayout.h"
#include "HidReport.h"

// Forward declaration
class I_InputController;

template <typename LayoutType>
class HidKeyboard : public I_HidKeyboardHandler {
public:
    HidKeyboard();
    void setInputController(I_InputController* controller);

    // --- Public API for Controllers ---
    bool isConnected() const;
    void requestToSend();
    bool sendChar(char character);
    void sendReport(const HidReport& report);
    bool charToKeypress(char character, Keypress& out_key) const;

    // --- Core Methods ---
    void setup();
    void powerOn();
    
    // --- I_HidKeyboardHandler Implementation ---
    void handlePacket(uint8_t packet_type, uint16_t, uint8_t* packet, uint16_t) override;
    void canSendNow() override;
#ifndef HAVE_BTSTACK_STDIN
    void typingTimerHandler(btstack_timer_source_t* ts) override;
#endif

private:
    enum class ProtocolMode : uint8_t { Boot = 0, Report = 1 };
    
    void setupServices();
    void setupAdvertisements();
    
    hci_con_handle_t m_connection_handle;
    ProtocolMode m_protocol_mode;
    LayoutType m_layout;
    I_InputController* m_input_controller = nullptr;
};

extern template class HidKeyboard<USKeyboardLayout>;

#endif // HID_KEYBOARD_H