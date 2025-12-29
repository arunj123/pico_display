#include "StatusScreen.h"
#include "font_freesans_16.h"
#include <cstring>
#include <cstdio>

// Colors
#define C_WHITE   0xFFFF
#define C_BLACK   0x0000
#define C_BLUE    0x0419 // Deep Blue
#define C_GREEN   0x0505 // Success Green
#define C_RED     0xC000 // Error Red
#define C_ORANGE  0xFC00
#define C_DARK    0x18C3 // Dark Gray/Blue bg

StatusScreen::StatusScreen(Drawing& drawing) : m_drawing(drawing) {}

void StatusScreen::show(Type type, const char* extra_text) {
    uint16_t bg_color = C_BLACK;
    uint16_t icon_color = C_WHITE;
    const char* title = "";
    const char* subtitle = "";

    switch (type) {
        case Type::BOOT:
            bg_color = C_BLACK;
            title = "SYSTEM BOOT";
            subtitle = "Initializing hardware...";
            break;
        case Type::WIFI_CONNECTING:
            bg_color = C_BLUE;
            title = "CONNECTING";
            subtitle = extra_text ? extra_text : "Joining Network...";
            break;
        case Type::WIFI_CONNECTED:
            bg_color = C_GREEN; 
            title = "ONLINE";
            subtitle = "Waiting for Host...";
            break;
        case Type::WIFI_ERROR:
            bg_color = C_RED;
            title = "CONNECTION FAILED";
            subtitle = "Check Wi-Fi Settings";
            break;
        case Type::SETUP_MODE:
            bg_color = C_ORANGE;
            title = "SETUP MODE";
            subtitle = "Connect via Bluetooth";
            break;
        case Type::DISCONNECTED:
            bg_color = C_DARK;
            title = "DISCONNECTED";
            subtitle = "Connection Lost";
            break;
    }

    uint16_t w = m_drawing.getWidth();
    uint16_t h = m_drawing.getHeight();
    uint16_t cx = w / 2;
    uint16_t cy = h / 2;

    // 1. Clear Screen
    m_drawing.fillRect(0, 0, w, h, bg_color);

    // 2. Draw Icon (Centered roughly in top half)
    uint16_t icon_y = cy - 50;
    switch (type) {
        case Type::WIFI_CONNECTING: drawIconWifi(cx, icon_y, icon_color); break;
        case Type::WIFI_CONNECTED:  drawIconCheck(cx, icon_y, icon_color); break;
        case Type::WIFI_ERROR:      drawIconError(cx, icon_y, icon_color); break;
        case Type::SETUP_MODE:      drawIconSetup(cx, icon_y, C_BLACK); break; // Black icon on Orange
        default:                    drawIconBoot(cx, icon_y, icon_color); break;
    }

    // 3. Draw Title (Centered)
    int title_len = strlen(title);
    // Approx width calculation: chars * 11px (avg width of freesans16)
    int title_w = title_len * 11; 
    m_drawing.drawString(cx - (title_w / 2), cy + 10, title, icon_color, &font_freesans_16);

    // 4. Draw Subtitle
    if (strlen(subtitle) > 0) {
        int sub_len = strlen(subtitle);
        int sub_w = sub_len * 9; // Slightly smaller est
        // Use a slightly dimmer color for subtitle if bg is dark, else black
        uint16_t sub_color = (type == Type::SETUP_MODE) ? C_BLACK : 0xBDF7; 
        m_drawing.drawString(cx - (sub_w / 2), cy + 40, subtitle, sub_color, &font_freesans_16);
    }

    // 5. Draw IP Address Box (Specific for Connected state)
    if (type == Type::WIFI_CONNECTED && extra_text) {
        // Draw a rounded-rect style box at the bottom
        int ip_len = strlen(extra_text);
        int ip_w = ip_len * 10;
        int box_x = cx - (ip_w / 2) - 10;
        int box_y = h - 40;
        int box_w = ip_w + 20;
        int box_h = 24;

        m_drawing.fillRect(box_x, box_y, box_w, box_h, 0x0200); // Dark green box
        m_drawing.drawString(cx - (ip_w / 2), box_y + 4, extra_text, C_WHITE, &font_freesans_16);
    }
}

// --- Simple Icon Primitives ---

void StatusScreen::drawIconWifi(uint16_t x, uint16_t y, uint16_t color) {
    // Draw dot
    m_drawing.fillRect(x-3, y+20, 6, 6, color);
    
    // Draw simple arcs (using rects for simplicity without a circle algo)
    // Small Arc
    m_drawing.fillRect(x-10, y+10, 4, 4, color); m_drawing.fillRect(x+6, y+10, 4, 4, color);
    m_drawing.fillRect(x-6, y+6, 12, 4, color);

    // Big Arc
    m_drawing.fillRect(x-18, y, 4, 4, color); m_drawing.fillRect(x+14, y, 4, 4, color);
    m_drawing.fillRect(x-14, y-4, 28, 4, color);
}

void StatusScreen::drawIconCheck(uint16_t x, uint16_t y, uint16_t color) {
    // Draw a checkmark
    for(int i=0; i<5; i++) {
        m_drawing.fillRect(x-15+i, y+i, 4, 4, color); // Down stroke
    }
    for(int i=0; i<15; i++) {
        m_drawing.fillRect(x-10+i, y+5-i, 4, 4, color); // Up stroke
    }
}

void StatusScreen::drawIconError(uint16_t x, uint16_t y, uint16_t color) {
    // Draw X
    for(int i=-15; i<15; i++) {
        m_drawing.fillRect(x+i, y+i, 3, 3, color);
        m_drawing.fillRect(x+i, y-i, 3, 3, color);
    }
}

void StatusScreen::drawIconSetup(uint16_t x, uint16_t y, uint16_t color) {
    // Draw a Gear-ish shape (Box with corners)
    m_drawing.fillRect(x-15, y-15, 30, 30, color);
    m_drawing.fillRect(x-5, y-5, 10, 10, C_ORANGE); // Hole
}

void StatusScreen::drawIconBoot(uint16_t x, uint16_t y, uint16_t color) {
    // Draw 3 dots
    m_drawing.fillRect(x-20, y, 8, 8, color);
    m_drawing.fillRect(x-4, y, 8, 8, color);
    m_drawing.fillRect(x+12, y, 8, 8, color);
}