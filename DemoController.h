#ifndef DEMO_CONTROLLER_H
#define DEMO_CONTROLLER_H

#include "I_InputController.h"
#include "HidKeyboard.h"

class DemoController : public I_InputController {
public:
    void init(HidKeyboard<USKeyboardLayout>* keyboard) override;
    void onReadyToSend() override;
    void typingTimerHandler(btstack_timer_source_t* ts) override;
    // --- FIX: Add override for the new interface method ---
    void onHidSubscribed() override;

private:
    static constexpr int TYPING_PERIOD_MS = 50;
    const char* m_demo_text = "\n\nHello World!\n\nThis is a C++ BTstack HID Keyboard.\n\n";
    
    HidKeyboard<USKeyboardLayout>* m_keyboard = nullptr;
    int m_demo_pos = 0;
    btstack_timer_source_t m_typing_timer;
    HidReport m_report_to_send;
    bool m_send_keyup = false;
};

#endif // DEMO_CONTROLLER_H