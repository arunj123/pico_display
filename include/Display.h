// File: Display.h

#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

enum class DisplayOrientation {
    PORTRAIT,
    LANDSCAPE
};

class St7789Display {
public:
    // UPDATED: Constructor no longer takes a backlight pin
    St7789Display(PIO pio, uint pin_sda, uint pin_scl, uint pin_cs, uint pin_dc, uint pin_reset, DisplayOrientation orientation);

    void fillScreen(uint16_t color);
    void drawBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* buffer, uint16_t fillColor = 0);
    
    uint16_t getWidth() const { return m_width; }
    uint16_t getHeight() const { return m_height; }

private:
    void send_command(const uint8_t* cmd, size_t count);
    void set_dc_cs(bool dc, bool cs);
    void init_display();

    PIO m_pio;
    uint m_sm;
    uint m_offset;

    uint m_pin_sda, m_pin_scl, m_pin_cs, m_pin_dc, m_pin_reset;
    
    uint16_t m_width;
    uint16_t m_height;
    DisplayOrientation m_orientation;
};

#endif // DISPLAY_H