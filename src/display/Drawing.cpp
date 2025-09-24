// File: Drawing.cpp

#include "Drawing.h"
#include <cstdlib> // For abs()

Drawing::Drawing(St7789Display& display) : m_display(display) {}

void Drawing::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    // Check bounds
    if (x >= m_display.getWidth() || y >= m_display.getHeight()) {
        return;
    }
    m_display.drawBuffer(x, y, 1, 1, &color);
}

void Drawing::fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    // Check bounds
    if (x >= m_display.getWidth() || y >= m_display.getHeight()) {
        return;
    }
    if ((x + width) > m_display.getWidth()) {
        width = m_display.getWidth() - x;
    }
    if ((y + height) > m_display.getHeight()) {
        height = m_display.getHeight() - y;
    }
    
    // Use the optimized fill method
    m_display.drawBuffer(x, y, width, height, nullptr, color);
}

void Drawing::drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    fillRect(x, y, width, 1, color); // Top
    fillRect(x, y + height - 1, width, 1, color); // Bottom
    fillRect(x, y, 1, height, color); // Left
    fillRect(x + width - 1, y, 1, height, color); // Right
}

// Bresenham's line algorithm
void Drawing::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = abs(x1 - x0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0);
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;

    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Drawing::drawChar(uint16_t x, uint16_t y, char c, uint16_t color, const custom_font_t* font) {
    if (c < font->first_char || c > font->last_char) {
        return;
    }

    // Calculate the offset to the character's bitmap data
    uint32_t char_offset = (c - font->first_char) * font->height * font->bytes_per_row;
    const uint8_t* char_data = font->data + char_offset;

    for (uint8_t j = 0; j < font->height; ++j) {
        for (uint8_t byte_idx = 0; byte_idx < font->bytes_per_row; ++byte_idx) {
            uint8_t line = char_data[j * font->bytes_per_row + byte_idx];
            
            for (uint8_t i = 0; i < 8; ++i) {
                uint8_t x_offset = byte_idx * 8 + i;
                if (x_offset >= font->width) {
                    continue; // Don't draw pixels beyond the character's width
                }

                if ((line >> i) & 1) {
                    drawPixel(x + x_offset, y + j, color);
                }
            }
        }
    }
}

void Drawing::drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, const custom_font_t* font) {
    uint16_t current_x = x;
    while (*str) {
        char c = *str++;
        drawChar(current_x, y, c, color, font);
        
        // Use the specific width for this character, plus a 1-pixel gap
        uint8_t char_width = font->widths[c - font->first_char];
        current_x += char_width + 1;
    }
}

void Drawing::drawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data) {
    // Check bounds
    if (x >= m_display.getWidth() || y >= m_display.getHeight()) {
        return;
    }
    // Simple wrapper around the low-level buffer drawing function
    m_display.drawBuffer(x, y, width, height, image_data);
}