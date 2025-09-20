#include "HidKeyboard.h"
#include "btstack_stdin.h"
#include <cstdio>

// Define the single, global instance of our keyboard class.
// The 'extern' declaration in HidKeyboard.cpp will see this.
HidKeyboard g_keyboard;

// C-style function to forward stdin processing to our class instance.
static void stdin_process_forwarder(char character) {
    g_keyboard.processStdin(character);
}

// The main entry point for the BTstack library.
// This is a C function, so it must be wrapped in extern "C".
extern "C" int btstack_main(void) {
    
    g_keyboard.setup();

#ifdef HAVE_BTSTACK_STDIN
    btstack_stdin_setup(stdin_process_forwarder);
#endif

    g_keyboard.powerOn();
    return 0;
}