#include "StdinController.h"
#include "btstack_stdin.h"
#include <cstdio>

static StdinController* g_stdin_instance = nullptr;

static void stdin_forwarder(char character) {
    if (g_stdin_instance) g_stdin_instance->processCharacter(character);
}

void StdinController::init(HidKeyboard<USKeyboardLayout>* keyboard) {
    m_keyboard = keyboard;
    g_stdin_instance = this;
    btstack_ring_buffer_init(&m_ascii_input_buffer, m_ascii_input_storage, sizeof(m_ascii_input_storage));
    btstack_stdin_setup(stdin_forwarder);
}

void StdinController::processCharacter(char character) {
    uint8_t c = character;
    btstack_ring_buffer_write(&m_ascii_input_buffer, &c, 1);
    
    if (m_state == State::Idle && m_keyboard && m_keyboard->isConnected()) {
        m_state = State::WaitingToSendKey;
        m_keyboard->requestToSend();
    }
}

void StdinController::typingTimerHandler(btstack_timer_source_t* ts) { (void)ts; }
void StdinController::onHidSubscribed() {}

void StdinController::onReadyToSend() {
    if (!m_keyboard) return;

    if (m_state == State::WaitingToSendKey) {
        uint8_t c;
        uint32_t num_bytes_read;
        btstack_ring_buffer_read(&m_ascii_input_buffer, &c, 1, &num_bytes_read);
        
        if (num_bytes_read == 0) {
            m_state = State::Idle;
            return;
        }

        printf("sending: %c\n", c);
        
        // --- FIX: Create the report directly in the controller ---
        Keypress key;
        if (m_keyboard->charToKeypress(c, key)) {
            HidReport report{};
            report.modifier = key.modifier;
            report.setKey(key.keycode);
            m_keyboard->sendReport(report);
            
            m_state = State::WaitingToSendKeyUp;
            m_keyboard->requestToSend();
        }
    } else if (m_state == State::WaitingToSendKeyUp) {
        m_keyboard->sendReport(HidReport::keyUp());
        
        if (btstack_ring_buffer_bytes_available(&m_ascii_input_buffer)) {
            m_state = State::WaitingToSendKey;
            m_keyboard->requestToSend();
        } else {
            m_state = State::Idle;
        }
    }
}