// File: Drawing.h

#ifndef DRAWING_H
#define DRAWING_H

#include "Display.h"
#include "CustomFont.h"

class Drawing {
public:
    Drawing(St7789Display& display);

    // Primitives
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
    void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

    // Text
    void drawChar(uint16_t x, uint16_t y, char c, uint16_t color, const custom_font_t* font);
    void drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, const custom_font_t* font);

    // Getters
    uint16_t getWidth() const { return m_display.getWidth(); }
    uint16_t getHeight() const { return m_display.getHeight(); }

private:
    St7789Display& m_display;
};

#endif // DRAWING_H