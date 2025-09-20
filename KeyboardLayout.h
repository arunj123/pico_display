#ifndef KEYBOARD_LAYOUT_H
#define KEYBOARD_LAYOUT_H

#include <cstdint>

// A struct to hold the result of a key lookup
struct Keypress {
    uint8_t keycode;
    uint8_t modifier;
};

// Abstract base class for all keyboard layouts
class KeyboardLayout {
public:
    virtual ~KeyboardLayout() = default;
    // Tries to find the keycode and modifier for a given character.
    // Returns true if found, false otherwise.
    virtual bool findKey(uint8_t character, Keypress& out_keypress) const = 0;
};


// Concrete implementation for a standard US Keyboard Layout
class USKeyboardLayout : public KeyboardLayout {
public:
    bool findKey(uint8_t character, Keypress& out_keypress) const override;
};


#endif // KEYBOARD_LAYOUT_H