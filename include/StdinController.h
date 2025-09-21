#ifndef STDIN_CONTROLLER_H
#define STDIN_CONTROLLER_H

#include "I_InputController.h"
#include "HidKeyboard.h"

class StdinController : public I_InputController {
public:
    void init(HidKeyboard<USKeyboardLayout>* keyboard) override;
    void onReadyToSend() override;
    void typingTimerHandler(btstack_timer_source_t* ts) override;
    // --- FIX: Add override for the new interface method ---
    void onHidSubscribed() override;
    
    void processCharacter(char character);

private:
    enum class State { Idle, WaitingToSendKey, WaitingToSendKeyUp };
    HidKeyboard<USKeyboardLayout>* m_keyboard = nullptr;
    State m_state = State::Idle;
    btstack_ring_buffer_t m_ascii_input_buffer;
    uint8_t m_ascii_input_storage[20];
};

#endif // STDIN_CONTROLLER_H