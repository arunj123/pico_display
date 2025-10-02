#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include "pico/stdlib.h"
#include "pico/sync.h"

class RotaryEncoder {
public:
    RotaryEncoder(uint pin_A, uint pin_B, uint pin_Key);
    void init(); // The new initialization method

    bool read_and_clear_raw_press_event();
    int8_t read_and_clear_rotation();

    // --- Getters for the static ISR ---
    uint get_pin_A() const { return m_pin_A; }
    uint get_pin_Key() const { return m_pin_Key; }

    // --- ISR methods need to be public now ---
    void _rotation_isr();
    void _key_isr();

private:
    uint m_pin_A, m_pin_B, m_pin_Key;
    volatile int8_t m_rotation_count = 0;
    volatile bool m_key_event_occurred = false;
    
    volatile uint64_t m_last_rotation_interrupt_time_us = 0;
    
    critical_section_t m_crit_sec;
};

#endif // ROTARY_ENCODER_H