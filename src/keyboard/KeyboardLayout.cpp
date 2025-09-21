#include "KeyboardLayout.h"
#include <array>
#include <cstddef> // Required for size_t

// Anonymous namespace to keep these constants local to this file
namespace {

// Using constexpr for key definitions
constexpr uint8_t CHAR_ILLEGAL     = 0xff;
constexpr uint8_t CHAR_RETURN      = '\n';
constexpr uint8_t CHAR_ESCAPE      = 27;
constexpr uint8_t CHAR_TAB         = '\t';
constexpr uint8_t CHAR_BACKSPACE   = 0x7f;

// Use std::array for type-safe, fixed-size arrays.
constexpr std::array<uint8_t, 101> keytable_us_none = {
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2', '3', '4', 
    '5', '6', '7', '8', '9', '0', CHAR_RETURN, CHAR_ESCAPE, CHAR_BACKSPACE, CHAR_TAB, ' ', '-', '=', '[', ']', 
    '\\', CHAR_ILLEGAL, ';', '\'', 0x60, ',', '.', '/', CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, 
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, 
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, 
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, '*', '-', '+', '\n', '1', '2', '3', '4', '5', '6', '7', '8', 
    '9', '0', '.', 0xa7, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL
};

constexpr std::array<uint8_t, 101> keytable_us_shift = {
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@', '#', '$', 
    '%', '^', '&', '*', '(', ')', CHAR_RETURN, CHAR_ESCAPE, CHAR_BACKSPACE, CHAR_TAB, ' ', '_', '+', '{', '}', 
    '|', CHAR_ILLEGAL, ':', '"', 0x7E, '<', '>', '?', CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, 
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, 
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, 
    CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, '*', '-', '+', '\n', '1', '2', '3', '4', '5', '6', '7', '8', 
    '9', '0', '.', 0xb1, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL, CHAR_ILLEGAL
};

// --- FIX: Replace std::span with a template function ---
// This function is instantiated at compile time for each array size it's called with.
// It is type-safe and has zero runtime overhead.
template <size_t N>
bool lookup_keycode(uint8_t character, const std::array<uint8_t, N>& table, uint8_t& keycode) {
    for (size_t i = 0; i < table.size(); ++i) {
        if (table[i] == character) {
            keycode = static_cast<uint8_t>(i);
            return true;
        }
    }
    return false;
}

} // anonymous namespace

bool USKeyboardLayout::findKey(uint8_t character, Keypress& out_keypress) const {
    // Try the non-shifted table first
    if (lookup_keycode(character, keytable_us_none, out_keypress.keycode)) {
        out_keypress.modifier = 0; // No modifier
        return true;
    }

    // Try the shifted table next
    if (lookup_keycode(character, keytable_us_shift, out_keypress.keycode)) {
        out_keypress.modifier = 2; // Left Shift
        return true;
    }
    
    return false;
}