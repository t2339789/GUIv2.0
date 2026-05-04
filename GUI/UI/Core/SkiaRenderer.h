#pragma once
#include <GL/gl.h>

class SkiaRenderer {
private:
    int width, height;

public:
    SkiaRenderer(int w, int h) : width(w), height(h) {}

    bool isValid() const { return width > 0 && height > 0; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void begin() {
        glPushAttrib(GL_ALL_ATTRIB_BITS);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, width, height, 0.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void flush() {
        glFlush();

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glPopAttrib();
    }
};
