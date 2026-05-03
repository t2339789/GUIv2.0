#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <jni.h>
#include <string>

class ClickGUI {
private:
    static bool isVisible;
    static bool wasInsertPressed;
    static bool wasZeroPressed;
    static bool wasLMBPressed;
    static bool wasRMBPressed;
    static int windowX, windowY;
    static int windowWidth, windowHeight;
    static bool isDragging;
    static int dragOffsetX, dragOffsetY;
    static int expandedModule;  // Index of module with settings panel open (-1 = none)
    static int activeSlider;    // Index of setting being dragged (-1 = none)

public:
    static void init();
    static void render(HDC hDc, JNIEnv* env);
    static bool getIsVisible() { return isVisible; }
    static void drawRect(float x, float y, float width, float height, float r, float g, float b, float a);
    static void drawText(JNIEnv* env, jobject fontRenderer, const char* text, int x, int y);
};
