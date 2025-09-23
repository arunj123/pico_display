// File: CustomFont.h

#ifndef CUSTOM_FONT_H
#define CUSTOM_FONT_H

#include "pico/stdlib.h"

// A custom font structure that supports characters wider than 8 pixels.
typedef struct {
    const uint8_t *data;
    const uint8_t *widths;
    uint8_t width;
    uint8_t height;
    uint8_t bytes_per_row; // Number of bytes for each horizontal line of pixels
    uint8_t first_char;
    uint8_t last_char;
} custom_font_t;

#endif // CUSTOM_FONT_H