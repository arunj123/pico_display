// File: RotaryEncoder.cpp

#include "RotaryEncoder.h"
// We no longer need irq.h for this, pico/sync.h is sufficient
#include "pio_rotary_encoder.pio.h"

// --- Static members and handlers ---
static RotaryEncoder* p_encoder_instance[NUM_PIOS] = {nullptr, nullptr};

static void pio0_irq_handler() {
    if (p_encoder_instance[0]) {
        p_encoder_instance[0]->isr();
    }
}

static void pio1_irq_handler() {
    if (p_encoder_instance[1]) {
        p_encoder_instance[1]->isr();
    }
}

// --- Class Implementation ---
RotaryEncoder::RotaryEncoder(PIO pio_instance, uint rotary_encoder_A) :
    pio(pio_instance),
    rotation(0)
{
    // --- Initialize the spin lock ---
    uint lock_num = spin_lock_claim_unused(true);
    lock = spin_lock_init(lock_num);

    // --- The rest of the setup is the same ---
    uint pio_idx = pio_get_index(pio);
    assert(pio_idx < NUM_PIOS);
    assert(!p_encoder_instance[pio_idx]);
    p_encoder_instance[pio_idx] = this;

    uint8_t rotary_encoder_B = rotary_encoder_A + 1;
    sm = pio_claim_unused_sm(pio, true);

    pio_gpio_init(pio, rotary_encoder_A);
    gpio_set_pulls(rotary_encoder_A, true, false);
    pio_gpio_init(pio, rotary_encoder_B);
    gpio_set_pulls(rotary_encoder_B, true, false);

    uint offset = pio_add_program(pio, &pio_rotary_encoder_program);
    pio_sm_config c = pio_rotary_encoder_program_get_default_config(offset);
    sm_config_set_in_pins(&c, rotary_encoder_A);
    sm_config_set_in_shift(&c, false, false, 0);

    // Note: We need the real hardware/irq.h for this part
    #include "hardware/irq.h"
    uint pio_irq = (pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0;
    irq_set_exclusive_handler(pio_irq, (pio == pio0) ? pio0_irq_handler : pio1_irq_handler);
    irq_set_enabled(pio_irq, true);
    
    pio_interrupt_clear(pio, 0);
    pio_interrupt_clear(pio, 1);
    pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
    pio_set_irq0_source_enabled(pio, pis_interrupt1, true);

    pio_sm_init(pio, sm, offset + 16, &c); 
    pio_sm_set_enabled(pio, sm, true);
}

void RotaryEncoder::set_rotation(int _rotation) {
    // --- Use the spin lock to protect the write ---
    uint32_t irq_status = spin_lock_blocking(lock);
    rotation = _rotation;
    spin_unlock(lock, irq_status);
}

int RotaryEncoder::get_rotation(void) {
    // --- Use the spin lock to protect the read ---
    uint32_t irq_status = spin_lock_blocking(lock);
    int value = rotation;
    spin_unlock(lock, irq_status);
    return value;
}

void RotaryEncoder::isr() {
    if (pio_interrupt_get(pio, 0)) {
        pio_interrupt_clear(pio, 0);
        rotation++; // Clockwise
    }
    if (pio_interrupt_get(pio, 1)) {
        pio_interrupt_clear(pio, 1);
        rotation--; // Counter-Clockwise
    }
}