// File: src/pico/RotaryEncoder.cpp

#include "RotaryEncoder.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "config.h"
#include <cstdio>

static RotaryEncoder* g_encoder_instance = nullptr;

// Raw IRQ handler that checks for events on registered pins.
static void shared_gpio_irq_handler() {
    if (g_encoder_instance) {
        uint pin_a = g_encoder_instance->get_pin_A();
        uint pin_key = g_encoder_instance->get_pin_Key();

        // Handle Pin A (Rotation)
        if (gpio_get_irq_event_mask(pin_a) & GPIO_IRQ_EDGE_FALL) {
            gpio_acknowledge_irq(pin_a, GPIO_IRQ_EDGE_FALL);
            g_encoder_instance->_rotation_isr();
        }

        // Handle Key Pin (Press)
        if (gpio_get_irq_event_mask(pin_key) & GPIO_IRQ_EDGE_FALL) {
            gpio_acknowledge_irq(pin_key, GPIO_IRQ_EDGE_FALL);
            g_encoder_instance->_key_isr();
        }
    }
}

// It only sets up internal state and does NOT touch hardware.
RotaryEncoder::RotaryEncoder(uint pin_A, uint pin_B, uint pin_Key) :
    m_pin_A(pin_A), m_pin_B(pin_B), m_pin_Key(pin_Key)
{
    assert(!g_encoder_instance);
    g_encoder_instance = this;
    critical_section_init(&m_crit_sec);
}

void RotaryEncoder::init()
{
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

    // --- Register our raw handler with a higher priority ---
    uint32_t irq_mask = (1u << m_pin_A) | (1u << m_pin_Key);

    // The CYW43 driver uses priority 0x40. By specifying a numerically
    // lower value (e.g., 0x30), we are telling the SDK that our handler
    // is higher priority and must be called, allowing it to coexist.
    const uint8_t ENCODER_IRQ_PRIORITY = 0x30;

    gpio_add_raw_irq_handler_with_order_priority_masked(
        irq_mask, 
        &shared_gpio_irq_handler,
        ENCODER_IRQ_PRIORITY
    );

    irq_set_enabled(IO_IRQ_BANK0, true);

    gpio_set_irq_enabled(m_pin_A, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(m_pin_Key, GPIO_IRQ_EDGE_FALL, true);
}

void RotaryEncoder::_key_isr() {
    m_key_event_occurred = true;
}

void RotaryEncoder::_rotation_isr() {
    // --- Restore the original, correct logic ---
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