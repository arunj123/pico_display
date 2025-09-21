#ifndef MEDIA_APPLICATION_H
#define MEDIA_APPLICATION_H

#include "MediaControllerDevice.h"
#include "RotaryEncoder.h"
#include "btstack.h"

class MediaApplication {
public:
    MediaApplication();
    void run();

private:
    // Timer callback handlers (must be static for C-style callbacks)
    static void polling_handler_forwarder(btstack_timer_source_t* ts);
    static void release_handler_forwarder(btstack_timer_source_t* ts);

    // Actual implementation of handlers
    void polling_handler();
    void release_handler();

    MediaControllerDevice m_media_controller;
    RotaryEncoder m_encoder;
    
    btstack_timer_source_t m_release_timer;
    btstack_timer_source_t m_polling_timer;
    
    int m_processed_rotation = 0;
};

#endif // MEDIA_APPLICATION_H