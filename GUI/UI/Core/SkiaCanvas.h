#pragma once
#include "Canvas.h"
#include <GL/gl.h>
#include <windows.h>
#include <string>
#include <cmath>

class SkiaCanvas : public Canvas {
public:
    void drawRect(float x, float y, float w, float h, float r, float g, float b, float a) override {
        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
        glEnd();
    }

    void drawRoundRect(float x, float y, float w, float h, float radius, float r, float g, float b, float a) override {
        if (radius <= 0.5f) {
            drawRect(x, y, w, h, r, g, b, a);
            return;
        }

        float maxRadius = (w < h ? w : h) * 0.5f;
        if (radius > maxRadius) radius = maxRadius;

        glColor4f(r, g, b, a);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x + w * 0.5f, y + h * 0.5f);

        const int segments = 8;
        drawCorner(x + w - radius, y + h - radius, radius, 0, segments);
        drawCorner(x + radius, y + h - radius, radius, 1, segments);
        drawCorner(x + radius, y + radius, radius, 2, segments);
        drawCorner(x + w - radius, y + radius, radius, 3, segments);
        glVertex2f(x + w, y + h - radius);
        glEnd();
    }

    void drawText(const std::string& text, float x, float y, float r, float g, float b, float a) override {
        static bool s_fontReady = false;
        static GLuint s_fontBase = 0;

        if (!s_fontReady) {
            HDC hdc = wglGetCurrentDC();
            if (hdc) {
                HFONT hFont = CreateFontA(
                    -14, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                    ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                    ANTIALIASED_QUALITY, FF_DONTCARE, "Segoe UI"
                );
                if (hFont) {
                    SelectObject(hdc, hFont);
                }
                s_fontBase = glGenLists(96);
                if (s_fontBase != 0 && wglUseFontBitmapsA(hdc, 32, 96, s_fontBase) != FALSE) {
                    s_fontReady = true;
                }
            }
        }

        if (!s_fontReady || s_fontBase == 0 || text.empty()) return;

        glColor4f(r, g, b, a);
        glRasterPos2f(x, y + 12.0f);
        glListBase(s_fontBase - 32);
        glCallLists((GLsizei)text.size(), GL_UNSIGNED_BYTE, text.c_str());
    }

private:
    static void drawCorner(float cx, float cy, float radius, int corner, int segments) {
        const float pi = 3.1415926535f;
        float start = 0.0f;
        if (corner == 0) start = 0.0f;
        if (corner == 1) start = pi * 0.5f;
        if (corner == 2) start = pi;
        if (corner == 3) start = pi * 1.5f;

        for (int i = 0; i <= segments; ++i) {
            float t = start + ((float)i / (float)segments) * pi * 0.5f;
            glVertex2f(cx + cosf(t) * radius, cy + sinf(t) * radius);
        }
    }
};
