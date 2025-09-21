#ifndef CONFIG_H
#define CONFIG_H

// --- Hardware Pins ---
constexpr uint ENCODER_PIN_A = 10;
// Note: Pin B is assumed to be Pin A + 1 in the RotaryEncoder class

// --- Application Behavior ---
constexpr int POLLING_INTERVAL_MS = 75;
constexpr int RELEASE_DELAY_MS = 50;

#endif // CONFIG_H