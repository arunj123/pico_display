#include "MediaControllerDevice.h"
#include "BtStackManager.h"
#include "btstack_run_loop.h" // Required for timer functions
#include <cstdio>

// --- Only include and define stdin functionality if it's enabled in the config ---
#ifdef HAVE_BTSTACK_STDIN
#include "btstack_stdin.h"

// Define a short delay in milliseconds for the auto-release
#define RELEASE_DELAY_MS 50

static MediaControllerDevice mediaController;
static btstack_timer_source_t release_timer;

// This function will be called by the timer to send the "key up" command
static void release_handler(btstack_timer_source_t *ts){
    (void)ts; // Unused parameter
    printf("Auto-releasing volume key\n");
    mediaController.release();
}

// This is the main input handler
static void stdin_handler(char character) {
    printf("Received: %c\n", character);

    // Stop any previously scheduled release timer, in case of rapid presses
    btstack_run_loop_remove_timer(&release_timer);

    switch (character) {
        case 'u':
        case 'U':
            printf("Volume Up\n");
            mediaController.increaseVolume();
            // After sending volume up, schedule a release in 50ms
            btstack_run_loop_set_timer(&release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&release_timer);
            break;
        case 'd':
        case 'D':
            printf("Volume Down\n");
            mediaController.decreaseVolume();
            // After sending volume down, schedule a release in 50ms
            btstack_run_loop_set_timer(&release_timer, RELEASE_DELAY_MS);
            btstack_run_loop_add_timer(&release_timer);
            break;
        default:
            // For any other key, we can just send an immediate release
            mediaController.release();
            break;
    }
}
#endif // HAVE_BTSTACK_STDIN


extern "C" int btstack_main(void) {
#ifndef HAVE_BTSTACK_STDIN
    static MediaControllerDevice mediaController;
#endif

#ifdef HAVE_BTSTACK_STDIN
    btstack_stdin_setup(stdin_handler);
    // Set the callback for our release timer
    btstack_run_loop_set_timer_handler(&release_timer, release_handler);
#endif

    BtStackManager::getInstance().registerHandler(&mediaController);
    
    mediaController.setup();
    mediaController.powerOn();
    return 0;
}