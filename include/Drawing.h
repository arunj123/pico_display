// File: Drawing.h

#ifndef DRAWING_H
#define DRAWING_H

#include "Display.h"
#include "CustomFont.h"

class Drawing {
public:
    // --- A status enum to track the drawing engine's state ---
    enum DrawStatus {
        IDLE,
        BUSY
    };

    Drawing(St7789Display& display);

    // Primitives (these remain blocking as they are very fast)
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
    void fillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

    // Text
    void drawChar(uint16_t x, uint16_t y, char c, uint16_t color, const custom_font_t* font);
    void drawString(uint16_t x, uint16_t y, const char* str, uint16_t color, const custom_font_t* font);

private:
    void drawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data);

public:
    // --- Asynchronous Drawing API ---

    /**
     * @brief Initiates a non-blocking draw operation.
     * This function sets up the drawing job and returns immediately.
     * @return True if the job was started, false if the driver is already busy.
     */
    bool drawImageAsync(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data);

    /**
     * @brief Processes one chunk of an active drawing job.
     * This MUST be called on every iteration of the main application loop.
     * @return The current status (IDLE or BUSY).
     */
    DrawStatus processDrawing();

    void drawImageBlocking(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t* image_data);

    // Getters
    uint16_t getWidth() const { return m_display.getWidth(); }
    uint16_t getHeight() const { return m_display.getHeight(); }

private:
    St7789Display& m_display;

    // --- NEW: State variables for the asynchronous drawing job ---
    DrawStatus m_status;
    uint16_t m_async_x, m_async_y, m_async_width, m_async_height;
    const uint16_t* m_async_data;
    int m_async_y_offset; // How many rows we've drawn so far
};

#endif // DRAWING_H