#pragma once
#include <string>

// Lightweight canvas interface for UI drawing
class Canvas {
public:
    virtual void drawRect(float x, float y, float w, float h, float r, float g, float b, float a) = 0;
    virtual void drawRoundRect(float x, float y, float w, float h, float radius, float r, float g, float b, float a) {
        drawRect(x, y, w, h, r, g, b, a);
    }
    virtual void drawText(const std::string& text, float x, float y, float r, float g, float b, float a) = 0;
    virtual void drawTextSize(const std::string& text, float x, float y, float size, float r, float g, float b, float a) {
        drawText(text, x, y, r, g, b, a);
    }
    virtual ~Canvas() {}
};
