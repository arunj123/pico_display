// File: include/Drawing.h

#ifndef DRAWING_H
#define DRAWING_H

#include "Display.h"
#include "CustomFont.h"
#include "config.h"
#include <vector>

class Drawing {
public:
    enum class DrawStatus {
        IDLE,
        BUSY
    };

    Drawing(St7789Display& display);

    void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
    void drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, const custom_font_t* font);
    
    bool drawImageAsync(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data);
    DrawStatus processDrawing();

private:
    void drawChar(uint16_t x, uint16_t y, char c, uint16_t color, const custom_font_t* font);
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data);
    
    St7789Display& m_display;
    DrawStatus m_status;
    uint16_t m_async_x, m_async_y, m_async_width, m_async_height;
    std::vector<uint16_t> m_async_pixel_buffer;
    int m_async_y_offset;
};

#endif // DRAWING_H