#pragma once
#include "Canvas.h"
#include <string>

// Lightweight immediate-mode helper for drawing consistent UI primitives.
class ScreenUI {
public:
    explicit ScreenUI(Canvas& canvas) : c(canvas) {}

    void beginPanel(float x, float y, float w, float h, const std::string& title) {
        px = x; py = y; pw = w; ph = h;
        c.drawRect(px - 3.0f, py - 3.0f, pw + 6.0f, ph + 6.0f, 0.02f, 0.03f, 0.05f, 0.82f);
        c.drawRect(px - 1.0f, py - 1.0f, pw + 2.0f, ph + 2.0f, 0.18f, 0.26f, 0.36f, 0.78f);
        c.drawRect(px, py, pw, ph, 0.06f, 0.07f, 0.09f, 0.96f);
        c.drawRect(px, py, pw, 28.0f, 0.08f, 0.12f, 0.19f, 1.0f);
        c.drawRect(px, py + 28.0f, pw, 2.0f, 0.14f, 0.62f, 0.46f, 0.95f);
        c.drawText(title, px + 10.0f, py + 8.0f, 0.90f, 0.95f, 0.99f, 1.0f);
        cursorY = py + 36.0f;
    }

    float rowModule(const std::string& label, bool enabled, bool hover, bool hasChildren, bool expanded) {
        const float rowH = 24.0f;
        const float rowX = px + 6.0f;
        const float rowW = pw - 12.0f;
        if (enabled) {
            c.drawRect(rowX, cursorY, rowW, rowH, 0.10f, 0.45f, 0.30f, 0.82f);
            c.drawRect(rowX, cursorY, 3.0f, rowH, 0.22f, 0.92f, 0.62f, 0.92f);
        } else {
            c.drawRect(rowX, cursorY, rowW, rowH, 0.11f, 0.12f, 0.14f, 0.78f);
            c.drawRect(rowX, cursorY, 3.0f, rowH, 0.24f, 0.26f, 0.30f, 0.68f);
        }
        if (hover) {
            c.drawRect(rowX, cursorY, rowW, rowH, 0.92f, 0.96f, 1.0f, 0.10f);
        }
        c.drawText(label, px + 14.0f, cursorY + 7.0f, 0.90f, 0.95f, 0.99f, 1.0f);
        // Dedicated module toggle switch on row right side.
        const float trackW = 26.0f;
        const float trackH = 12.0f;
        const float trackX = px + pw - 44.0f;
        const float trackY = cursorY + 6.0f;
        if (enabled) c.drawRect(trackX, trackY, trackW, trackH, 0.91f, 0.19f, 0.29f, 0.92f);
        else c.drawRect(trackX, trackY, trackW, trackH, 0.24f, 0.26f, 0.30f, 0.95f);
        const float knobX = enabled ? (trackX + trackW - 9.0f) : (trackX + 1.0f);
        c.drawRect(knobX, trackY + 1.0f, 8.0f, 10.0f, 0.92f, 0.95f, 0.98f, 1.0f);
        if (hasChildren) {
            c.drawText(expanded ? "v" : ">", px + pw - 56.0f, cursorY + 7.0f, 0.80f, 0.88f, 0.95f, 0.95f);
        }
        float y = cursorY;
        cursorY += 26.0f;
        return y;
    }

    void beginSettingsBlock(int rows) {
        c.drawRect(px + 10.0f, cursorY, pw - 20.0f, (float)(rows * 30 + 6), 0.08f, 0.09f, 0.11f, 0.94f);
        c.drawRect(px + 10.0f, cursorY, pw - 20.0f, 1.0f, 0.22f, 0.30f, 0.38f, 0.7f);
        cursorY += 5.0f;
    }

    float rowSliderLabel(const std::string& label, float value) {
        c.drawText(label + ": " + toFixed2(value), px + 16.0f, cursorY + 3.0f, 0.85f, 0.90f, 0.95f, 0.95f);
        return cursorY;
    }

    void rowSliderTrack(float value, float minVal, float maxVal) {
        const float sliderX = px + 16.0f;
        const float sliderW = pw - 36.0f;
        const float sliderY = cursorY + 19.0f;
        const float sliderH = 5.0f;
        float pct = (value - minVal) / (maxVal - minVal);
        if (pct < 0.0f) pct = 0.0f;
        if (pct > 1.0f) pct = 1.0f;

        c.drawRect(sliderX, sliderY, sliderW, sliderH, 0.16f, 0.17f, 0.20f, 1.0f);
        c.drawRect(sliderX, sliderY, sliderW * pct, sliderH, 0.18f, 0.70f, 0.56f, 1.0f);
        c.drawRect(sliderX + sliderW * pct - 4.0f, sliderY - 3.0f, 8.0f, 11.0f, 0.90f, 0.95f, 0.98f, 1.0f);
    }

    void rowToggle(const std::string& label, bool enabled) {
        const float rowX = px + 12.0f;
        const float rowY = cursorY + 2.0f;
        const float rowW = pw - 24.0f;
        const float rowH = 24.0f;
        c.drawRect(rowX, rowY, rowW, rowH, 0.10f, 0.11f, 0.14f, 0.96f);
        c.drawText(label, px + 16.0f, cursorY + 8.0f, 0.84f, 0.89f, 0.94f, 0.95f);

        // Switch track + thumb (binary immediate control style)
        const float trackW = 30.0f;
        const float trackH = 14.0f;
        const float trackX = px + pw - 16.0f - trackW;
        const float trackY = cursorY + 7.0f;
        if (enabled) c.drawRect(trackX, trackY, trackW, trackH, 0.20f, 0.74f, 0.55f, 1.0f);
        else c.drawRect(trackX, trackY, trackW, trackH, 0.24f, 0.26f, 0.30f, 1.0f);

        const float thumb = 12.0f;
        const float thumbX = enabled ? (trackX + trackW - thumb - 1.0f) : (trackX + 1.0f);
        c.drawRect(thumbX, trackY + 1.0f, thumb, thumb, 0.92f, 0.95f, 0.98f, 1.0f);
    }

    void rowSegmented(const std::string& label, const char* const* options, int optionCount, int selectedIndex) {
        if (!options || optionCount <= 0) return;
        c.drawText(label, px + 16.0f, cursorY + 3.0f, 0.84f, 0.89f, 0.94f, 0.95f);
        const float segX = px + 16.0f;
        const float segY = cursorY + 16.0f;
        const float segW = pw - 32.0f;
        const float segH = 12.0f;
        c.drawRect(segX, segY, segW, segH, 0.14f, 0.16f, 0.20f, 1.0f);
        const float cellW = segW / (float)optionCount;
        for (int i = 0; i < optionCount; ++i) {
            float x = segX + cellW * (float)i;
            if (i == selectedIndex) c.drawRect(x + 1.0f, segY + 1.0f, cellW - 2.0f, segH - 2.0f, 0.18f, 0.70f, 0.56f, 1.0f);
            if (i > 0) c.drawRect(x, segY, 1.0f, segH, 0.24f, 0.28f, 0.34f, 0.9f);
        }
    }

    void nextRow() { cursorY += 30.0f; }
    float currentY() const { return cursorY; }
    float panelX() const { return px; }
    float panelW() const { return pw; }

private:
    Canvas& c;
    float px = 0.0f, py = 0.0f, pw = 0.0f, ph = 0.0f, cursorY = 0.0f;

    static std::string toFixed2(float value) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", value);
        return std::string(buf);
    }
};
