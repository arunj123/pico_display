#include "btstack_stdin.h"
#include "HidKeyboard.h" // Now includes the full template implementation
#include "KeyboardLayout.h"

// --- FIX: Instantiate the template with the desired layout ---
HidKeyboard<USKeyboardLayout> g_keyboard;

// C-style function to forward stdin processing to our class instance.
static void stdin_process_forwarder(char character) {
    g_keyboard.processStdin(character);
}

// The main entry point for the BTstack library.
extern "C" int btstack_main(void) {
    
    g_keyboard.setup();

#ifdef HAVE_BTSTACK_STDIN
    btstack_stdin_setup(stdin_process_forwarder);
#endif

    g_keyboard.powerOn();
    return 0;
}