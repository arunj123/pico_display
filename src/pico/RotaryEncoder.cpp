// File: src/pico/RotaryEncoder.cpp

#include "RotaryEncoder.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/time.h"
#include "config.h"

// A static pointer to the single instance of the encoder.
// This is necessary for the C-style ISR to call the C++ method.
static RotaryEncoder* g_encoder_instance = nullptr;

RotaryEncoder::RotaryEncoder(uint pin_A, uint pin_B, uint pin_Key) :
    m_pin_A(pin_A), m_pin_B(pin_B), m_pin_Key(pin_Key)
{
    assert(!g_encoder_instance);
    g_encoder_instance = this;

    critical_section_init(&m_crit_sec);

    // --- Init GPIOs ---
    gpio_init(m_pin_A);
    gpio_set_dir(m_pin_A, GPIO_IN);
    gpio_pull_up(m_pin_A);

    gpio_init(m_pin_B);
    gpio_set_dir(m_pin_B, GPIO_IN);
    gpio_pull_up(m_pin_B);

    gpio_init(m_pin_Key);
    gpio_set_dir(m_pin_Key, GPIO_IN);
    gpio_pull_up(m_pin_Key);

    // --- Init Interrupts ---
    // The shared handler will be called for any of these pins.
    // Trigger rotation ISR on the falling edge of Pin A.
    // Trigger key ISR on the falling edge of the Key pin.
    gpio_set_irq_enabled_with_callback(
        m_pin_A, GPIO_IRQ_EDGE_FALL, true, &RotaryEncoder::gpio_irq_handler);
        
    gpio_set_irq_enabled(
        m_pin_Key, GPIO_IRQ_EDGE_FALL, true);
}

// Static C-style ISR that forwards to the C++ class instance
void RotaryEncoder::gpio_irq_handler(uint gpio, uint32_t events) {
    if (g_encoder_instance) {
        if (gpio == g_encoder_instance->m_pin_A) {
            g_encoder_instance->_rotation_isr();
        } else if (gpio == g_encoder_instance->m_pin_Key) {
            g_encoder_instance->_key_isr();
        }
    }
}

void RotaryEncoder::_key_isr() {
    m_key_event_occurred = true;
}

void RotaryEncoder::_rotation_isr() {
    uint64_t now_us = time_us_64();
    if ((now_us - m_last_rotation_interrupt_time_us) < (DEBOUNCE_DELAY_MS_ROTATION * 1000)) {
        return;
    }
    critical_section_enter_blocking(&m_crit_sec);
    if (gpio_get(m_pin_B) == 0) m_rotation_count--;
    else m_rotation_count++;
    critical_section_exit(&m_crit_sec);
    m_last_rotation_interrupt_time_us = now_us;
}

bool RotaryEncoder::read_and_clear_raw_press_event() {
    critical_section_enter_blocking(&m_crit_sec);
    bool pressed = m_key_event_occurred;
    m_key_event_occurred = false;
    critical_section_exit(&m_crit_sec);
    return pressed;
}

int8_t RotaryEncoder::read_and_clear_rotation() {
    critical_section_enter_blocking(&m_crit_sec);
    int8_t count = m_rotation_count;
    m_rotation_count = 0;
    critical_section_exit(&m_crit_sec);
    return count;
}