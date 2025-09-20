#include "MediaControllerDevice.h"
#include "BtStackManager.h"
#include <cstdio>

// --- Only include and define stdin functionality if it's enabled in the config ---
#ifdef HAVE_BTSTACK_STDIN
#include "btstack_stdin.h"

static MediaControllerDevice mediaController;

// Simple stdin handler for media control
static void stdin_handler(char character) {
    printf("Received: %c\n", character);
    switch (character) {
        case 'u':
        case 'U':
            printf("Volume Up\n");
            mediaController.increaseVolume();
            break;
        case 'd':
        case 'D':
            printf("Volume Down\n");
            mediaController.decreaseVolume();
            break;
        default:
            // On any other key, release the volume buttons
            mediaController.release();
            break;
    }
}
#endif // HAVE_BTSTACK_STDIN


extern "C" int btstack_main(void) {
#ifndef HAVE_BTSTACK_STDIN
    // If stdin is not enabled, we still need a device object,
    // but we can't control it.
    static MediaControllerDevice mediaController;
#endif

#ifdef HAVE_BTSTACK_STDIN
    // Setup the callback for stdin if it is enabled
    btstack_stdin_setup(stdin_handler);
#endif

    BtStackManager::getInstance().registerHandler(&mediaController);
    
    mediaController.setup();
    mediaController.powerOn();
    return 0;
}