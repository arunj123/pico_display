// File: config.h

#ifndef CONFIG_H
#define CONFIG_H

#include "pico/stdlib.h"

// --- Rotary encoder Wiring ---
constexpr uint ENCODER_PIN_A = 10;
constexpr uint ENCODER_PIN_B = 11;
constexpr uint ENCODER_PIN_KEY = 12;

// --- Application Behavior ---
constexpr int RELEASE_DELAY_MS = 100;
constexpr int DEBOUNCE_DELAY_MS_KEY = 50; // A standard, robust debounce time
constexpr int DEBOUNCE_DELAY_MS_ROTATION = 2; // Short debounce for rotation

// --- Display pins for GMT020-02 in a clean, sequential order ---
// Wire your display according to this block
constexpr uint DISPLAY_PIN_CS    = 20;  // Chip Select
constexpr uint DISPLAY_PIN_SCL   = 16;  // Serial Clock
constexpr uint DISPLAY_PIN_SDA   = 17;  // Serial Data (DIN)
constexpr uint DISPLAY_PIN_DC    = 19;  // Data/Command (RS)
constexpr uint DISPLAY_PIN_RESET = 18;  // Reset
// The BL (Backlight) pin is not used for this display

// The maximum number of pixels the async drawing buffer can hold.
// A full-screen tile (320x12) is 3840 pixels. 4096 is a safe, round number.
constexpr size_t MAX_DRAW_BUFFER_PIXELS = 4096;

#include "private_config.h"

#endif // CONFIG_H
