#ifndef STATUS_SCREEN_H
#define STATUS_SCREEN_H

#include "Drawing.h"
#include <cstdint>

class StatusScreen {
public:
    enum class Type {
        BOOT,
        WIFI_CONNECTING,
        WIFI_CONNECTED,
        WIFI_ERROR,
        SETUP_MODE,
        DISCONNECTED
    };

    explicit StatusScreen(Drawing& drawing);

    void show(Type type, const char* extra_text = nullptr);

private:
    Drawing& m_drawing;

    // Helper drawing functions
    void drawIconWifi(uint16_t x, uint16_t y, uint16_t color);
    void drawIconError(uint16_t x, uint16_t y, uint16_t color);
    void drawIconCheck(uint16_t x, uint16_t y, uint16_t color);
    void drawIconSetup(uint16_t x, uint16_t y, uint16_t color);
    void drawIconBoot(uint16_t x, uint16_t y, uint16_t color);
};

#endif // STATUS_SCREEN_H