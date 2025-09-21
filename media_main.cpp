// File: media_main.cpp

#include "MediaControllerDevice.h"
#include "BtStackManager.h"
#include "btstack_run_loop.h"
#include "RotaryEncoder.h"
#include <cstdio>

// --- Define hardware pins and polling rate ---
constexpr uint ENCODER_PIN_A = 10;
constexpr int POLLING_INTERVAL_MS = 75;  // Poll a little faster for smoother response
constexpr int RELEASE_DELAY_MS = 50;

// --- Application Objects ---
static MediaControllerDevice mediaController;
static RotaryEncoder encoder(pio1, ENCODER_PIN_A); 

// --- Timers and State ---
static btstack_timer_source_t release_timer;
static btstack_timer_source_t polling_timer;
static int processed_rotation = 0; // Keeps track of the value we've sent commands for

// This function is called by the timer to send the "key up" command
static void release_handler(btstack_timer_source_t *ts){
    (void)ts; // Unused parameter
    if (mediaController.isConnected()) {
        mediaController.release();
    }
}

// This is the main polling handler for the rotary encoder
static void polling_handler(btstack_timer_source_t *ts) {
    // Only process if we are connected to a host
    if (mediaController.isConnected()) {
        int current_rotation = encoder.get_rotation();

        // Check if we need to send a "Volume Up" command
        if (current_rotation > processed_rotation) {
            printf("Volume Up (Target: %d, Processed: %d)\n", current_rotation, processed_rotation);
            mediaController.increaseVolume();
            btstack_run_loop_set_timer(&release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&release_timer);
            processed_rotation++; // We have now processed this single step up
        } 
        // Check if we need to send a "Volume Down" command
        else if (current_rotation < processed_rotation) {
            printf("Volume Down (Target: %d, Processed: %d)\n", current_rotation, processed_rotation);
            mediaController.decreaseVolume();
            btstack_run_loop_set_timer(&release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&release_timer);
            processed_rotation--; // We have now processed this single step down
        }
    }

    // Always reset the polling timer for the next check
    btstack_run_loop_set_timer(ts, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(ts);
}

extern "C" int btstack_main(void) {
    // Initialize the processed rotation to the encoder's starting value
    processed_rotation = encoder.get_rotation();

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