#include "btstack_stdin.h"
#include "HidKeyboard.h"
#include "BtStackManager.h"

// Define the keyboard object. It is no longer a global used by callbacks.
static HidKeyboard<USKeyboardLayout> keyboard;

// stdin processing now uses a pointer to our keyboard object
static void stdin_process_forwarder(char character) {
    keyboard.processStdin(character);
}

// The main entry point for the BTstack library
extern "C" int btstack_main(void) {
    
    // Get the manager singleton and register our keyboard as the handler
    BtStackManager::getInstance().registerHandler(&keyboard);
    
    // Setup the keyboard logic
    keyboard.setup();

#ifdef HAVE_BTSTACK_STDIN
    btstack_stdin_setup(stdin_process_forwarder);
#endif

    // Power on the bluetooth stack
    keyboard.powerOn();
    return 0;
}