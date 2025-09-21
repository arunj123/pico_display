// File: media_main.cpp

#include "MediaControllerDevice.h"
#include "BtStackManager.h"
#include "btstack_run_loop.h"
#include "RotaryEncoder.h" // Include our new encoder class
#include <cstdio>

// --- Define hardware pins and polling rate ---
constexpr uint ENCODER_PIN_A = 10;
constexpr int POLLING_INTERVAL_MS = 100;
constexpr int RELEASE_DELAY_MS = 50;

// --- Application Objects ---
static MediaControllerDevice mediaController;
static RotaryEncoder encoder(pio1, ENCODER_PIN_A); // Use the free pio1

// --- Timers ---
static btstack_timer_source_t release_timer;
static btstack_timer_source_t polling_timer;
static int last_rotation = 0;

// This function will be called by the timer to send the "key up" command
static void release_handler(btstack_timer_source_t *ts){
    (void)ts; // Unused parameter
    mediaController.release();
}

// This is the main polling handler for the rotary encoder
static void polling_handler(btstack_timer_source_t *ts) {
    int current_rotation = encoder.get_rotation();
    int delta = current_rotation - last_rotation;

    if (delta > 0) { // Clockwise rotation
        printf("Volume Up\n");
        mediaController.increaseVolume();
        // After sending volume up, schedule a release
        btstack_run_loop_set_timer(&release_timer, RELEASE_DELAY_MS);
        btstack_run_loop_add_timer(&release_timer);
    } else if (delta < 0) { // Counter-clockwise rotation
        printf("Volume Down\n");
        mediaController.decreaseVolume();
        // After sending volume down, schedule a release
        btstack_run_loop_set_timer(&release_timer, RELEASE_DELAY_MS);
        btstack_run_loop_add_timer(&release_timer);
    }

    last_rotation = current_rotation;

    // Reset the polling timer for the next check
    btstack_run_loop_set_timer(ts, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(ts);
}

extern "C" int btstack_main(void) {
    // Set up the auto-release timer
    btstack_run_loop_set_timer_handler(&release_timer, release_handler);
    
    // Set up and start the encoder polling timer
    btstack_run_loop_set_timer_handler(&polling_timer, polling_handler);
    btstack_run_loop_set_timer(&polling_timer, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(&polling_timer);

    // Register the media controller with the BTstack manager
    BtStackManager::getInstance().registerHandler(&mediaController);
    
    // Setup and power on the device
    mediaController.setup();
    mediaController.powerOn();
    return 0;
}