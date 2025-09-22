// File: Display.h

#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

class St7789Display {
public:
    St7789Display(PIO pio, uint pin_din, uint pin_clk, uint pin_cs, uint pin_dc, uint pin_reset, uint pin_bl);

    void fillScreen(uint16_t color);

    // It uses a default parameter for fillColor to handle both cases.
    void drawBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* buffer, uint16_t fillColor = 0);

private:
    void send_command(const uint8_t* cmd, size_t count);
    void set_dc_cs(bool dc, bool cs);
    void init_display();

    PIO m_pio;
    uint m_sm;
    uint m_offset;

    uint m_pin_din;
    uint m_pin_clk;
    uint m_pin_cs;
    uint m_pin_dc;
    uint m_pin_reset;
    uint m_pin_bl;
};

#endif // DISPLAY_H