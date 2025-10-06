// File: src/media/media_main.cpp

#include "MediaApplication.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"
#include "btstack_run_loop.h"
#include "btstack_event.h"

// A minimal packet handler to show BTstack status.
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(size);
    UNUSED(channel);
    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) == BTSTACK_EVENT_STATE) {
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
            bd_addr_t local_addr;
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
        }
    }
}

int main() {
    stdio_init_all();
    
    static MediaApplication app;

    if (cyw43_arch_init()) {
        printf("PANIC: Failed to initialize CYW43 Architecture\n");
        return -1;
    }

    // Register a basic handler for core BTstack events.
    static btstack_packet_callback_registration_t hci_event_callback_registration;
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Run the application's specific setup routine.
    app.setup();

    // Turn on the Bluetooth radio.
    hci_power_control(HCI_POWER_ON);
    
    // Start the BTstack scheduler. This function handles all initialization
    // and runs forever.
    btstack_run_loop_execute();
    
    return 0;
}