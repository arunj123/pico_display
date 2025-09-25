// File: include/pico/RotaryEncoder.h

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include "pico/stdlib.h"
#include "pico/sync.h"

class RotaryEncoder {
public:
    // Constructor now takes both pins
    RotaryEncoder(uint pin_A, uint pin_B);

    // This method will be called from the GPIO ISR
    void process_state_change();

    // These methods remain, but their internal locking might change
    void set_rotation(int _rotation);
    int get_rotation(void);

private:
    uint m_pin_A;
    uint m_pin_B;
    
    // For state-table decoding
    volatile int m_rotation;
    volatile uint8_t m_last_state;

    // A critical section is a better choice than a spin-lock for core-safe variable access
    critical_section_t m_crit_sec;
};

#endif // ROTARY_ENCODER_H
