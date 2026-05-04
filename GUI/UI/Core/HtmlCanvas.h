#pragma once
#include "Canvas.h"
#include <windows.h>
#include <GL/gl.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <memory>

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

class HtmlCanvas : public Canvas {
public:
    HtmlCanvas(int w, int h) : width(w), height(h) {
        bitmap = std::make_unique<Gdiplus::Bitmap>(width, height, PixelFormat32bppARGB);
        graphics = std::make_unique<Gdiplus::Graphics>(bitmap.get());
        graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
        graphics->SetCompositingMode(Gdiplus::CompositingModeSourceOver);
        graphics->SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
        graphics->Clear(Gdiplus::Color(0, 0, 0, 0));
    }

    void drawRect(float x, float y, float w, float h, float r, float g, float b, float a) override {
        Gdiplus::SolidBrush brush(color(r, g, b, a));
        graphics->FillRectangle(&brush, x, y, w, h);
    }

    void drawRoundRect(float x, float y, float w, float h, float radius, float r, float g, float b, float a) override {
        if (radius <= 0.5f) {
            drawRect(x, y, w, h, r, g, b, a);
            return;
        }
        Gdiplus::GraphicsPath path;
        roundedPath(path, x, y, w, h, radius);
        Gdiplus::SolidBrush brush(color(r, g, b, a));
        graphics->FillPath(&brush, &path);
    }

    void drawText(const std::string& text, float x, float y, float r, float g, float b, float a) override {
        drawTextSize(text, x, y, 13.0f, r, g, b, a);
    }

    void drawTextSize(const std::string& text, float x, float y, float size, float r, float g, float b, float a) override {
        if (text.empty()) return;
        std::wstring wide(text.begin(), text.end());
        Gdiplus::FontFamily family(L"Segoe UI");
        Gdiplus::Font font(&family, size, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brush(color(r, g, b, a));
        Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());
        fmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
        Gdiplus::PointF point(x, y);
        graphics->DrawString(wide.c_str(), -1, &font, point, &fmt, &brush);
    }

    void flushToOpenGL() {
        Gdiplus::Rect rect(0, 0, width, height);
        Gdiplus::BitmapData data{};
        if (bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data) != Gdiplus::Ok) return;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data.Scan0);

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f((float)width, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f((float)width, (float)height);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, (float)height);
        glEnd();

        bitmap->UnlockBits(&data);
        glDeleteTextures(1, &tex);
        glDisable(GL_TEXTURE_2D);
    }

private:
    int width;
    int height;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    std::unique_ptr<Gdiplus::Graphics> graphics;

    static Gdiplus::Color color(float r, float g, float b, float a) {
        auto cv = [](float v) -> BYTE {
            v = std::max(0.0f, std::min(1.0f, v));
            return (BYTE)std::lround(v * 255.0f);
        };
        return Gdiplus::Color(cv(a), cv(r), cv(g), cv(b));
    }

    static void roundedPath(Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float radius) {
        radius = std::min(radius, std::min(w, h) * 0.5f);
        const float d = radius * 2.0f;
        path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
        path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
        path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
        path.AddArc(x, y, d, d, 180.0f, 90.0f);
        path.CloseFigure();
    }
};
