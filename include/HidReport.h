#ifndef HID_REPORT_H
#define HID_REPORT_H

#include <cstdint>
#include <cstring> // For memset

// Represents the 8-byte boot protocol report for a standard HID keyboard.
// This struct ensures type safety and clarifies the structure of the data being sent.
struct HidReport {
    // --- Member Variables ---

    uint8_t modifier = 0;
    uint8_t reserved = 0;
    uint8_t keycodes[6] = {0};

    // --- Helper Methods ---

    // Creates an empty "key up" report.
    static HidReport keyUp() {
        return HidReport{}; // Returns a default-initialized report (all zeros)
    }
    
    // Sets the primary keycode for the report.
    void setKey(uint8_t keycode) {
        keycodes[0] = keycode;
    }

    // Returns a pointer to the raw byte data for sending.
    const uint8_t* data() const {
        return reinterpret_cast<const uint8_t*>(this);
    }

    // Returns the size of the report in bytes.
    static constexpr size_t size() {
        return sizeof(HidReport);
    }
};

#endif // HID_REPORT_H