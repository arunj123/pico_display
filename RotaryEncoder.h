// File: RotaryEncoder.h

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "pico/sync.h" // <-- Add this header

class RotaryEncoder {
public:
    RotaryEncoder(PIO pio_instance, uint rotary_encoder_A);

    void set_rotation(int _rotation);
    int get_rotation(void);
    void isr();

private:
    PIO pio;
    uint sm;
    volatile int rotation;
    spin_lock_t* lock; 
};

#endif // ROTARY_ENCODER_H