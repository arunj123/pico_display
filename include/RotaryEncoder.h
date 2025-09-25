// File: include/pico/RotaryEncoder.h

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include "pico/stdlib.h"
#include "pico/sync.h"

class RotaryEncoder {
public:
    RotaryEncoder(uint pin_A, uint pin_B, uint pin_Key);

    // Renamed for clarity: this gets the RAW state.
    bool read_and_clear_raw_press_event();
    int8_t read_and_clear_rotation();

private:
    static void gpio_irq_handler(uint gpio, uint32_t events);

    // ISRs now just update volatile flags
    void _key_isr();
    void _rotation_isr();

    uint m_pin_A, m_pin_B, m_pin_Key;
    volatile int8_t m_rotation_count = 0;
    volatile bool m_key_event_occurred = false; // Renamed for clarity
    
    // We only need one timer for rotation, key debounce is now in the main loop
    volatile uint64_t m_last_rotation_interrupt_time_us = 0;
    
    critical_section_t m_crit_sec;
};

#endif // ROTARY_ENCODER_H