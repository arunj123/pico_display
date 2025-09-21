#ifndef CONFIG_H
#define CONFIG_H

#include "pico/stdlib.h"

// --- Hardware Pins ---
constexpr uint ENCODER_PIN_A = 10;
// Note: Pin B is assumed to be Pin A + 1 in the RotaryEncoder class
constexpr uint ENCODER_PIN_KEY = 12;

// --- Application Behavior ---
constexpr int POLLING_INTERVAL_MS = 75;
constexpr int RELEASE_DELAY_MS = 50;
constexpr int DEBOUNCE_DELAY_MS = 50;

#endif // CONFIG_H