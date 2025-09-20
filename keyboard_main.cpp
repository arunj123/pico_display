#include "HidKeyboard.h"
#include "BtStackManager.h"
#include "I_InputController.h"

#ifdef HAVE_BTSTACK_STDIN
#include "StdinController.h"
#else
#include "DemoController.h"
#endif

extern "C" int btstack_main(void) {
    static HidKeyboard<USKeyboardLayout> keyboard;
#ifdef HAVE_BTSTACK_STDIN
    static StdinController inputController;
#else
    static DemoController inputController;
#endif

    keyboard.setInputController(&inputController);
    inputController.init(&keyboard);
    BtStackManager::getInstance().registerHandler(&keyboard);
    
    keyboard.setup();
    keyboard.powerOn();
    return 0;
}