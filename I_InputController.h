#ifndef I_INPUT_CONTROLLER_H
#define I_INPUT_CONTROLLER_H

#include "btstack.h"

// Forward declare to avoid circular dependencies
template<typename LayoutType>
class HidKeyboard;
class USKeyboardLayout;

// Interface for classes that control the input to the HID Keyboard
class I_InputController {
public:
    virtual ~I_InputController() = default;

    virtual void init(HidKeyboard<USKeyboardLayout>* keyboard) = 0;
    virtual void onReadyToSend() = 0;
    virtual void typingTimerHandler(btstack_timer_source_t* ts) = 0;
    
    // --- FIX: Add a method to notify the controller that the host is ready ---
    virtual void onHidSubscribed() = 0;
};

#endif // I_INPUT_CONTROLLER_H