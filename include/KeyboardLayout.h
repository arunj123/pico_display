#ifndef KEYBOARD_LAYOUT_H
#define KEYBOARD_LAYOUT_H

#include <cstdint>

// A struct to hold the result of a key lookup
struct Keypress {
    uint8_t keycode;
    uint8_t modifier;
};

// --- FIX: The abstract base class is no longer needed for templates ---

// Concrete implementation for a standard US Keyboard Layout
class USKeyboardLayout {
public:
    // The "interface" is now defined by the methods the template expects.
    // In this case, HidKeyboard will expect a findKey method with this signature.
    bool findKey(uint8_t character, Keypress& out_keypress) const;
};


#endif // KEYBOARD_LAYOUT_H