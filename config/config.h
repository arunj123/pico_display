// File: config.h

#ifndef CONFIG_H
#define CONFIG_H

#include "pico/stdlib.h"

// --- Hardware Pins ---
constexpr uint ENCODER_PIN_A = 10;
constexpr uint ENCODER_PIN_KEY = 12;

// --- Application Behavior ---
constexpr int POLLING_INTERVAL_MS = 75;
constexpr int RELEASE_DELAY_MS = 50;
constexpr int DEBOUNCE_DELAY_MS = 100;
// How many encoder "counts" per single volume step.
// Most encoders are 4 counts per physical click.
constexpr int ENCODER_COUNTS_PER_STEP = 4;

// --- Display pins ---
constexpr uint DISPLAY_PIN_DIN   = 0;  // SDA
constexpr uint DISPLAY_PIN_CLK   = 1;  // SCL
constexpr uint DISPLAY_PIN_CS    = 2;  // CS
constexpr uint DISPLAY_PIN_DC    = 3;  // RS (DC)
constexpr uint DISPLAY_PIN_RESET = 4;  // RESET
constexpr uint DISPLAY_PIN_BL    = 5;  // Backlight Enable

#endif // CONFIG_H