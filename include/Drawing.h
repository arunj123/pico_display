// File: include/Drawing.h

#ifndef DRAWING_H
#define DRAWING_H

#include "Display.h"
#include "CustomFont.h"
#include <array>
#include <cstdlib> // For abs()
#include <cstring> // For memcpy

template <size_t MaxBufferSizeInPixels>
class Drawing {
public:
    enum class DrawStatus {
        IDLE,
        BUSY
    };

    Drawing(St7789Display& display) : 
        m_display(display),
        m_status(DrawStatus::IDLE),
        m_async_y_offset(0) 
    {}

    // Primitives
    void drawPixel(uint16_t x, uint16_t y, uint16_t color) {
        if (x >= m_display.getWidth() || y >= m_display.getHeight()) return;
        m_display.drawBuffer(x, y, 1, 1, &color);
    }

    void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
        if (x >= m_display.getWidth() || y >= m_display.getHeight()) return;
        if ((x + width) > m_display.getWidth()) width = m_display.getWidth() - x;
        if ((y + height) > m_display.getHeight()) height = m_display.getHeight() - y;
        m_display.drawBuffer(x, y, width, height, nullptr, color);
    }

    void drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
        fillRect(x, y, width, 1, color);
        fillRect(x, y + height - 1, width, 1, color);
        fillRect(x, y, 1, height, color);
        fillRect(x + width - 1, y, 1, height, color);
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int16_t err = dx + dy;
        while (true) {
            drawPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            int16_t e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void drawChar(uint16_t x, uint16_t y, char c, uint16_t color, const custom_font_t* font) {
        if (c < font->first_char || c > font->last_char) return;
        uint32_t char_offset = (c - font->first_char) * font->height * font->bytes_per_row;
        const uint8_t* char_data = font->data + char_offset;
        for (uint8_t j = 0; j < font->height; ++j) {
            for (uint8_t byte_idx = 0; byte_idx < font->bytes_per_row; ++byte_idx) {
                uint8_t line = char_data[j * font->bytes_per_row + byte_idx];
                for (uint8_t i = 0; i < 8; ++i) {
                    uint8_t x_offset = byte_idx * 8 + i;
                    if (x_offset >= font->width) continue;
                    if ((line >> i) & 1) drawPixel(x + x_offset, y + j, color);
                }
            }
        }
    }

    void drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, const custom_font_t* font) {
        uint16_t current_x = x;
        while (*str) {
            char c = *str++;
            drawChar(current_x, y, c, color, font);
            uint8_t char_width = font->widths[c - font->first_char];
            current_x += char_width + 1;
        }
    }

    // --- Asynchronous Drawing API ---
    bool drawImageAsync(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data) {
        if (m_status == DrawStatus::BUSY) return false;
        
        size_t pixel_count = (size_t)width * height;
        if (pixel_count > MaxBufferSizeInPixels) return false; // Safety check

        m_status = DrawStatus::BUSY;
        m_async_x = x;
        m_async_y = y;
        m_async_width = width;
        m_async_height = height;
        m_async_y_offset = 0;
        
        memcpy(m_async_pixel_buffer.data(), image_data, pixel_count * sizeof(uint16_t));

        return true;
    }

    DrawStatus processDrawing() {
        if (m_status == DrawStatus::IDLE) return DrawStatus::IDLE;

        const int ROWS_PER_CHUNK = 4;
        int rows_remaining = m_async_height - m_async_y_offset;
        int rows_to_draw = (rows_remaining < ROWS_PER_CHUNK) ? rows_remaining : ROWS_PER_CHUNK;

        if (rows_to_draw > 0) {
            uint16_t start_y = m_async_y + m_async_y_offset;
            const uint16_t* chunk_pixel_data = m_async_pixel_buffer.data() + ((size_t)m_async_y_offset * m_async_width);
            drawImage(m_async_x, start_y, m_async_width, rows_to_draw, chunk_pixel_data);
            m_async_y_offset += rows_to_draw;
        }

        if (m_async_y_offset >= m_async_height) {
            m_status = DrawStatus::IDLE;
        }
        return m_status;
    }

    void drawImageBlocking(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data) {
        drawImage(x, y, width, height, image_data);
    }

    uint16_t getWidth() const { return m_display.getWidth(); }
    uint16_t getHeight() const { return m_display.getHeight(); }

private:
    void drawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data) {
        if (x >= m_display.getWidth() || y >= m_display.getHeight()) return;
        m_display.drawBuffer(x, y, width, height, image_data);
    }

    St7789Display& m_display;
    DrawStatus m_status;
    uint16_t m_async_x, m_async_y, m_async_width, m_async_height;
    std::array<uint16_t, MaxBufferSizeInPixels> m_async_pixel_buffer;
    int m_async_y_offset;
};

#endif // DRAWING_H