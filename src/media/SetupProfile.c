// File: src/media/SetupProfile.c
#include <stdint.h>

// --- FIX: Rename profile_data to avoid collision with the main application ---
#define profile_data setup_profile_data
#include "setup_mode.h"
#undef profile_data

// Expose the Setup Profile data to the C++ code
const uint8_t * get_setup_profile_data(void) {
    return setup_profile_data; // Return the renamed variable
}

// Expose the specific handle for the Wi-Fi characteristic
uint16_t get_setup_wifi_handle(void) {
    return ATT_CHARACTERISTIC_0000FF01_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE;
}