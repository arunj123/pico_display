#include "DemoController.h"
#include "BtStackManager.h"
#include <cstdio>

void DemoController::init(HidKeyboard<USKeyboardLayout>* keyboard) {
    // --- FIX: init should only store the keyboard pointer ---
    m_keyboard = keyboard;
}

// --- FIX: The logic to start the demo now lives here ---
void DemoController::onHidSubscribed() {
    printf("Start typing demo...\n");
    m_typing_timer.process = &BtStackManager::typingTimerForwarder;
    btstack_run_loop_set_timer(&m_typing_timer, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(&m_typing_timer);
}

void DemoController::typingTimerHandler(btstack_timer_source_t* ts) {
    if (!m_keyboard || !m_keyboard->isConnected()) return;

    if (m_send_keyup) {
        m_send_keyup = false;
        m_report_to_send = HidReport::keyUp();
        m_keyboard->requestToSend();
    } else {
        uint8_t character = m_demo_text[m_demo_pos++];
        if (m_demo_text[m_demo_pos] == 0) m_demo_pos = 0;
        
        Keypress key;
        if (m_keyboard->charToKeypress(character, key)) {
            printf("%c\n", character);
            m_report_to_send = HidReport{};
            m_report_to_send.modifier = key.modifier;
            m_report_to_send.setKey(key.keycode);
            m_send_keyup = true;
            m_keyboard->requestToSend();
        }
    }
    btstack_run_loop_set_timer(ts, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

void DemoController::onReadyToSend() {
    if (m_keyboard) {
        m_keyboard->sendReport(m_report_to_send);
    }
}