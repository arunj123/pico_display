#include "HidKeyboard.h"
#include "BtStackManager.h"
#include "I_InputController.h"

// --- Include the concrete controller headers ---
#ifdef HAVE_BTSTACK_STDIN
#include "StdinController.h"
#else
#include "DemoController.h"
#endif

// The main entry point for the BTstack library
extern "C" int btstack_main(void) {
    // 1. Create the application objects
    static HidKeyboard<USKeyboardLayout> keyboard;
    
#ifdef HAVE_BTSTACK_STDIN
    static StdinController inputController;
#else
    static DemoController inputController;
#endif

    // 2. Connect the components
    keyboard.setInputController(&inputController);
    inputController.init(&keyboard);
    BtStackManager::getInstance().registerHandler(&keyboard);
    
    // 3. Setup and run
    keyboard.setup();
    keyboard.powerOn();
    return 0;
}