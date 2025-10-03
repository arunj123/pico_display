// File: Drawing.cpp

#include "Drawing.h"
#include <cstdlib> // For abs()

Drawing::Drawing(St7789Display& display) : 
    m_display(display),
    m_status(IDLE),
    m_async_y_offset(0) 
{}

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

// It's the low-level function that the async processor will call for each chunk
void Drawing::drawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data) {
    // Check bounds
    if (x >= m_display.getWidth() || y >= m_display.getHeight()) {
        return;
    }
    m_display.drawBuffer(x, y, width, height, image_data);
}

void Drawing::drawImageBlocking(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data) {
    // This public function is just a wrapper around the private, low-level implementation.
    drawImage(x, y, width, height, image_data);
}

bool Drawing::drawImageAsync(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data) {
    // Don't start a new job if one is already in progress
    if (m_status == BUSY) {
        return false;
    }

    // Set up the state for the new job
    m_status = BUSY;
    m_async_x = x;
    m_async_y = y;
    m_async_width = width;
    m_async_height = height;
    m_async_data = image_data;
    m_async_y_offset = 0; // Reset progress

    return true;
}

Drawing::DrawStatus Drawing::processDrawing() {
    // If there's no active job, do nothing.
    if (m_status == IDLE) {
        return IDLE;
    }

    // --- This is the core chunking logic, now encapsulated within the driver ---
    const int ROWS_PER_CHUNK = 4; // A good balance of speed and responsiveness

    int rows_remaining = m_async_height - m_async_y_offset;
    int rows_to_draw = (rows_remaining < ROWS_PER_CHUNK) ? rows_remaining : ROWS_PER_CHUNK;

    if (rows_to_draw > 0) {
        // Calculate the starting position and data pointer for this specific chunk
        uint16_t start_y = m_async_y + m_async_y_offset;
        const uint16_t* chunk_pixel_data = m_async_data + (m_async_y_offset * m_async_width);

        // Call the low-level blocking function for just this small chunk
        drawImage(m_async_x, start_y, m_async_width, rows_to_draw, chunk_pixel_data);

        // Update our progress
        m_async_y_offset += rows_to_draw;
    }

    // Check if we have finished drawing the entire image
    if (m_async_y_offset >= m_async_height) {
        m_status = IDLE; // The job is done, switch back to IDLE
    }

    return m_status;
}