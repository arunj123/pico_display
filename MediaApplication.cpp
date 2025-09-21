// File: MediaApplication.cpp

#include "MediaApplication.h"
#include "BtStackManager.h"
#include "config.h"
#include <cstdio>

MediaApplication::MediaApplication() : 
    m_encoder(pio1, ENCODER_PIN_A)
{
    // Set the context for the timers to be this object instance
    m_polling_timer.context = this;
    m_release_timer.context = this;
    
    // Initialize the processed rotation to the encoder's starting value
    m_processed_rotation = m_encoder.get_rotation();

    // Set up the timer callback handlers
    btstack_run_loop_set_timer_handler(&m_release_timer, &MediaApplication::release_handler_forwarder);
    btstack_run_loop_set_timer_handler(&m_polling_timer, &MediaApplication::polling_handler_forwarder);
    
    // Register the media controller with the BTstack manager
    BtStackManager::getInstance().registerHandler(&m_media_controller);
}

void MediaApplication::run() {
    m_media_controller.setup();
    m_media_controller.powerOn();

    // Start the main polling timer
    btstack_run_loop_set_timer(&m_polling_timer, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(&m_polling_timer);

    // This is a blocking call that starts the event loop
    btstack_run_loop_execute();
}

// --- Static Forwarders to bridge C-style callbacks to C++ methods ---
void MediaApplication::polling_handler_forwarder(btstack_timer_source_t* ts) {
    MediaApplication* app = static_cast<MediaApplication*>(ts->context);
    if (app) {
        app->polling_handler();
    }
}

void MediaApplication::release_handler_forwarder(btstack_timer_source_t* ts) {
    MediaApplication* app = static_cast<MediaApplication*>(ts->context);
    if (app) {
        app->release_handler();
    }
}

// --- Member Function Implementations ---
void MediaApplication::polling_handler() {
    // Only process if we are connected to a host
    if (m_media_controller.isConnected()) {
        int current_rotation = m_encoder.get_rotation();

        // CORRECT LOGIC: Use "if/else if" to process only ONE step per poll event
        if (current_rotation > m_processed_rotation) {
            printf("Volume Up (Target: %d, Processed: %d)\n", current_rotation, m_processed_rotation);
            m_media_controller.increaseVolume();
            btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_release_timer);
            m_processed_rotation++; // We have now processed this single step up
        } 
        else if (current_rotation < m_processed_rotation) {
            printf("Volume Down (Target: %d, Processed: %d)\n", current_rotation, m_processed_rotation);
            m_media_controller.decreaseVolume();
            btstack_run_loop_set_timer(&m_release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&m_release_timer);
            m_processed_rotation--; // We have now processed this single step down
        }
    }

    // Always reset the polling timer for the next check
    btstack_run_loop_set_timer(&m_polling_timer, POLLING_INTERVAL_MS);
    btstack_run_loop_add_timer(&m_polling_timer);
}

void MediaApplication::release_handler() {
    if (m_media_controller.isConnected()) {
        m_media_controller.release();
    }
}