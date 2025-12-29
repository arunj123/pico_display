#pragma once

#include <cstdint>
#include "Drawing.h"

class SetupDrawing {
public:
    // Initialize with a reference to the Drawing driver
    explicit SetupDrawing(Drawing& drawing);

    // Draw the Setup QR Code and instructions
    void showSetupScreen(const char* url);

private:
    Drawing& m_drawing;
};