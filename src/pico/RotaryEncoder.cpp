// File: src/pico/RotaryEncoder.cpp

#include "RotaryEncoder.h"
#include "hardware/gpio.h"
#include <cstdio>

// The state table for the rotary encoder logic, as inspired by your sample.
// (State << 2) | Current gives an index into this table.
static const int8_t state_table[] = {
  0, -1,  1,  0,  // State 00 -> 00, 01, 10, 11
  1,  0,  0, -1,  // State 01 -> 00, 01, 10, 11
 -1,  0,  0,  1,  // State 10 -> 00, 01, 10, 11
  0,  1, -1,  0   // State 11 -> 00, 01, 10, 11
};

RotaryEncoder::RotaryEncoder(uint pin_A, uint pin_B) :
    m_pin_A(pin_A), m_pin_B(pin_B), m_rotation(0) 
{
    // Initialize the critical section for thread/IRQ safety
    critical_section_init(&m_crit_sec);

    // Initialize GPIOs for input with pull-ups
    gpio_init(m_pin_A);
    gpio_set_dir(m_pin_A, GPIO_IN);
    gpio_pull_up(m_pin_A);
    
    gpio_init(m_pin_B);
    gpio_set_dir(m_pin_B, GPIO_IN);
    gpio_pull_up(m_pin_B);

    // Initialize the starting state
    m_last_state = (gpio_get(m_pin_A) << 1) | gpio_get(m_pin_B);
}

void RotaryEncoder::process_state_change() {
    // Read the current state of the pins
    uint8_t current_A = gpio_get(m_pin_A);
    uint8_t current_B = gpio_get(m_pin_B);
    uint8_t current_state = (current_A << 1) | current_B;

    // If the state hasn't changed, do nothing (debounce)
    if (current_state == m_last_state) {
        return;
    }
    
    // Look up the transition in the state table
    int8_t change = state_table[(m_last_state << 2) | current_state];
    
    if (change != 0) {
        // We use a critical section to safely modify the rotation count
        critical_section_enter_blocking(&m_crit_sec);
        m_rotation += change;
        critical_section_exit(&m_crit_sec);
    }
    
    // Update the last state
    m_last_state = current_state;
}

void RotaryEncoder::set_rotation(int _rotation) {
    critical_section_enter_blocking(&m_crit_sec);
    m_rotation = _rotation;
    critical_section_exit(&m_crit_sec);
}

int RotaryEncoder::get_rotation(void) {
    critical_section_enter_blocking(&m_crit_sec);
    int value = m_rotation;
    critical_section_exit(&m_crit_sec);
    return value;
}
