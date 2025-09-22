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

#endif // CONFIG_H