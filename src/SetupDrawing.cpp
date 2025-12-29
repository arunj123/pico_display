#include "SetupDrawing.h"
#include "qrcodegen.h"
#include <cstdio>

SetupDrawing::SetupDrawing(Drawing& drawing) : m_drawing(drawing) {
}

void SetupDrawing::showSetupScreen(const char* url) {
    printf("[SetupDrawing] Generating QR Code for: %s\n", url);

    // Static buffers to avoid stack overflow
    static uint8_t qr0[qrcodegen_BUFFER_LEN_MAX];
    static uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    
    // Generate QR
    bool ok = qrcodegen_encodeText(url, tempBuffer, qr0, qrcodegen_Ecc_LOW,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

    if (!ok) {
        printf("[SetupDrawing] Error: QR Code generation failed\n");
        return;
    }

    int size = qrcodegen_getSize(qr0);
    uint16_t screen_w = m_drawing.getWidth();
    uint16_t screen_h = m_drawing.getHeight();

    // 1. Clear Screen to White
    m_drawing.fillRect(0, 0, screen_w, screen_h, 0xFFFF); 

    // 2. Calculate Scale
    int padding = 10;
    int avail_size = (screen_w < screen_h ? screen_w : screen_h) - (padding * 2);
    int scale = avail_size / size;
    if (scale < 1) scale = 1;

    // 3. Center
    int start_x = (screen_w - (size * scale)) / 2;
    int start_y = (screen_h - (size * scale)) / 2;

    // 4. Draw Modules
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qrcodegen_getModule(qr0, x, y)) {
                // Draw Black Module
                m_drawing.fillRect(start_x + (x * scale), start_y + (y * scale), scale, scale, 0x0000);
            }
        }
    }
    printf("[SetupDrawing] QR Code Drawn. Size: %dx%d, Scale: %d\n", size, size, scale);
}