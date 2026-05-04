#include "ClickGUI.h"
#include "ModuleManager.h"
#include "Modules/StatCheckerModule.h"
#include <jni.h>
#include <sstream>
#include <iomanip>

extern jclass mcClass;
extern jfieldID mcInstanceField;
extern jfieldID fontRendererField;
extern jmethodID drawStringMethod;
extern jfieldID locationFontTextureField;
extern jfieldID renderEngineField;
extern jmethodID bindTextureMethod;

typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
static PFNGLACTIVETEXTUREPROC glActiveTexturePtr = nullptr;
static PFNGLUSEPROGRAMPROC glUseProgramPtr = nullptr;
static bool glFuncsInitialized = false;

void initGlFuncs() {
    if (glFuncsInitialized) return;
    glActiveTexturePtr = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
    glUseProgramPtr = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    glFuncsInitialized = true;
}

bool ClickGUI::isVisible = false;
bool ClickGUI::wasInsertPressed = false;
bool ClickGUI::wasZeroPressed = false;
bool ClickGUI::wasLMBPressed = false;
bool ClickGUI::wasRMBPressed = false;
int ClickGUI::windowX = 50;
int ClickGUI::windowY = 50;
int ClickGUI::windowWidth = 260;
int ClickGUI::windowHeight = 300;
bool ClickGUI::isDragging = false;
int ClickGUI::dragOffsetX = 0;
int ClickGUI::dragOffsetY = 0;
int ClickGUI::expandedModule = -1;
int ClickGUI::activeSlider = -1;

void ClickGUI::init() {}

void ClickGUI::drawRect(float x, float y, float width, float height, float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void ClickGUI::drawText(JNIEnv* env, jobject fontRenderer, const char* text, int x, int y) {
    if (!env || !fontRenderer || !text || !drawStringMethod) return;

    initGlFuncs();
    if (glActiveTexturePtr) glActiveTexturePtr(0x84C0);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);

    if (locationFontTextureField && renderEngineField && bindTextureMethod) {
        jobject locObj = env->GetObjectField(fontRenderer, locationFontTextureField);
        if (locObj) {
            jobject renderEngineObj = env->GetObjectField(fontRenderer, renderEngineField);
            if (renderEngineObj) {
                env->CallVoidMethod(renderEngineObj, bindTextureMethod, locObj);
                env->DeleteLocalRef(renderEngineObj);
            }
            env->DeleteLocalRef(locObj);
        }
        env->ExceptionClear();
    }

    jstring jstr = env->NewStringUTF(text);
    env->CallIntMethod(fontRenderer, drawStringMethod, jstr, (jint)x, (jint)y, (jint)0xFFFFFFFF);
    env->DeleteLocalRef(jstr);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);
}

void ClickGUI::render(HDC hDc, JNIEnv* env) {
    bool isInsertPressed = (GetAsyncKeyState(VK_INSERT) & 0x8000);
    bool isZeroPressed = (GetAsyncKeyState(0x30) & 0x8000);
    bool isCtrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000);

    if ((isInsertPressed && !wasInsertPressed) || (isZeroPressed && !wasZeroPressed && isCtrlDown)) {
        isVisible = !isVisible;
    }
    wasInsertPressed = isInsertPressed;
    wasZeroPressed = isZeroPressed;

    bool isTabPressed = (GetAsyncKeyState(VK_TAB) & 0x8000);
    StatCheckerModule* statChecker = nullptr;
    bool statCheckerEnabled = false;

    if (isTabPressed) {
        for (Module* mod : ModuleManager::getModules()) {
            if (mod->getName() == "StatChecker") {
                statChecker = (StatCheckerModule*)mod;
                statCheckerEnabled = statChecker->isEnabled();
                break;
            }
        }
    }

    if (!isVisible && !statCheckerEnabled) return;
    if (!mcClass) return;

    jobject mc = env->GetStaticObjectField(mcClass, mcInstanceField);
    if (!mc) return;
    jobject fontRenderer = env->GetObjectField(mc, fontRendererField);
    if (!fontRenderer) {
        env->DeleteLocalRef(mc);
        return;
    }

    HWND hwnd = WindowFromDC(hDc);
    if (!hwnd) {
        env->DeleteLocalRef(fontRenderer);
        env->DeleteLocalRef(mc);
        return;
    }

    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);
    int mouseX = p.x;
    int mouseY = p.y;

    bool isLMBPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
    bool isRMBPressed = (GetAsyncKeyState(VK_RBUTTON) & 0x8000);
    bool clicked = isLMBPressed && !wasLMBPressed;
    bool rightClicked = isRMBPressed && !wasRMBPressed;

    if (clicked && isVisible) {
        if (mouseX >= windowX && mouseX <= windowX + windowWidth &&
            mouseY >= windowY && mouseY <= windowY + 28) {
            isDragging = true;
            dragOffsetX = mouseX - windowX;
            dragOffsetY = mouseY - windowY;
        }
    }

    if (!isLMBPressed) {
        isDragging = false;
        activeSlider = -1;
    }
    if (isDragging) {
        windowX = mouseX - dragOffsetX;
        windowY = mouseY - dragOffsetY;
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glOrtho(0, viewport[2], viewport[3], 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initGlFuncs();

    GLint lastProgram = 0;
    GLint lastActiveTexture = 0;
    if (glUseProgramPtr) glGetIntegerv(0x8B8D, &lastProgram);
    if (glActiveTexturePtr) glGetIntegerv(0x84E0, &lastActiveTexture);
    if (glUseProgramPtr) glUseProgramPtr(0);
    if (glActiveTexturePtr) glActiveTexturePtr(0x84C0);

    if (isVisible) {
        std::vector<Module*>& modules = ModuleManager::getModules();
        int totalHeight = 46;
        for (int i = 0; i < (int)modules.size(); i++) {
            totalHeight += 26;
            if (expandedModule == i) {
                totalHeight += (int)modules[i]->getSettings().size() * 30 + 10;
            }
        }
        windowHeight = totalHeight;

        drawRect((float)windowX - 4, (float)windowY - 4, (float)windowWidth + 8, (float)windowHeight + 8, 0.02f, 0.03f, 0.05f, 0.82f);
        drawRect((float)windowX - 1, (float)windowY - 1, (float)windowWidth + 2, (float)windowHeight + 2, 0.18f, 0.26f, 0.36f, 0.78f);
        drawRect((float)windowX, (float)windowY, (float)windowWidth, (float)windowHeight, 0.06f, 0.07f, 0.09f, 0.96f);
        drawRect((float)windowX, (float)windowY, (float)windowWidth, 28.0f, 0.08f, 0.12f, 0.19f, 1.0f);
        drawRect((float)windowX, (float)windowY + 28.0f, (float)windowWidth, 2.0f, 0.14f, 0.62f, 0.46f, 0.95f);
        drawRect((float)windowX + 1.0f, (float)windowY + 1.0f, (float)windowWidth - 2.0f, 1.0f, 0.42f, 0.56f, 0.72f, 0.45f);
        drawText(env, fontRenderer, "Poltergeist", windowX + 10, windowY + 9);

        int startY = windowY + 36;
        for (int i = 0; i < (int)modules.size(); i++) {
            Module* mod = modules[i];
            bool hover = (mouseX >= windowX + 7 && mouseX <= windowX + windowWidth - 7 &&
                          mouseY >= startY && mouseY <= startY + 24);

            if (clicked && hover) {
                mod->toggle();
            }
            if (rightClicked && hover && !mod->getSettings().empty()) {
                expandedModule = (expandedModule == i) ? -1 : i;
            }

            if (mod->isEnabled()) {
                drawRect((float)windowX + 6, (float)startY, (float)windowWidth - 12, 24.0f, 0.10f, 0.45f, 0.30f, 0.82f);
                drawRect((float)windowX + 6, (float)startY, 3.0f, 24.0f, 0.22f, 0.92f, 0.62f, 0.92f);
            } else {
                drawRect((float)windowX + 6, (float)startY, (float)windowWidth - 12, 24.0f, 0.11f, 0.12f, 0.14f, 0.78f);
                drawRect((float)windowX + 6, (float)startY, 3.0f, 24.0f, 0.24f, 0.26f, 0.30f, 0.68f);
            }

            if (hover) {
                drawRect((float)windowX + 6, (float)startY, (float)windowWidth - 12, 24.0f, 0.92f, 0.96f, 1.0f, 0.10f);
            }

            drawText(env, fontRenderer, mod->getName().c_str(), windowX + 14, startY + 8);

            if (!mod->getSettings().empty()) {
                const char* arrow = (expandedModule == i) ? "v" : ">";
                drawText(env, fontRenderer, arrow, windowX + windowWidth - 20, startY + 8);
            }

            startY += 26;

            if (expandedModule == i) {
                drawRect((float)windowX + 10, (float)startY, (float)windowWidth - 20, (float)(mod->getSettings().size() * 30 + 6), 0.08f, 0.09f, 0.11f, 0.94f);
                drawRect((float)windowX + 10, (float)startY, (float)windowWidth - 20, 1.0f, 0.22f, 0.30f, 0.38f, 0.7f);
                startY += 5;

                auto& settings = mod->getSettings();
                for (int j = 0; j < (int)settings.size(); j++) {
                    Setting& s = settings[j];

                    if (s.type == Setting::FLOAT_SLIDER) {
                        std::stringstream ss;
                        ss << s.name << ": " << std::fixed << std::setprecision(2) << s.value;
                        drawText(env, fontRenderer, ss.str().c_str(), windowX + 16, startY + 3);

                        float sliderX = (float)windowX + 16;
                        float sliderW = (float)windowWidth - 36;
                        float sliderY = (float)startY + 19;
                        float sliderH = 5.0f;

                        drawRect(sliderX, sliderY, sliderW, sliderH, 0.16f, 0.17f, 0.20f, 1.0f);

                        float pct = (s.value - s.minVal) / (s.maxVal - s.minVal);
                        if (pct < 0.0f) pct = 0.0f;
                        if (pct > 1.0f) pct = 1.0f;
                        drawRect(sliderX, sliderY, sliderW * pct, sliderH, 0.18f, 0.70f, 0.56f, 1.0f);

                        float knobX = sliderX + sliderW * pct - 4;
                        drawRect(knobX, sliderY - 3, 8, 11, 0.90f, 0.95f, 0.98f, 1.0f);

                        bool sliderHover = (mouseX >= (int)sliderX && mouseX <= (int)(sliderX + sliderW) &&
                                            mouseY >= (int)(sliderY - 4) && mouseY <= (int)(sliderY + sliderH + 6));

                        if (clicked && sliderHover) {
                            activeSlider = j + i * 100;
                        }
                        if (isLMBPressed && activeSlider == j + i * 100) {
                            float newPct = ((float)mouseX - sliderX) / sliderW;
                            if (newPct < 0.0f) newPct = 0.0f;
                            if (newPct > 1.0f) newPct = 1.0f;
                            s.value = s.minVal + newPct * (s.maxVal - s.minVal);
                        }
                    } else if (s.type == Setting::BOOL_TOGGLE) {
                        float boxX = (float)windowX + 16;
                        float boxY = (float)startY + 5;

                        if (s.getBool()) {
                            drawRect(boxX, boxY, 12, 12, 0.18f, 0.72f, 0.50f, 1.0f);
                            drawRect(boxX + 2, boxY + 2, 8, 8, 0.90f, 1.0f, 0.95f, 0.95f);
                        } else {
                            drawRect(boxX, boxY, 12, 12, 0.20f, 0.22f, 0.25f, 1.0f);
                        }
                        drawText(env, fontRenderer, s.name.c_str(), windowX + 34, startY + 7);

                        bool toggleHover = (mouseX >= (int)boxX && mouseX <= (int)(boxX + 12) &&
                                            mouseY >= (int)boxY && mouseY <= (int)(boxY + 12));
                        if (clicked && toggleHover) {
                            s.setBool(!s.getBool());
                        }
                    }

                    startY += 30;
                }
            }
        }
    }

    if (statCheckerEnabled && statChecker) {
        statChecker->drawStats(env, fontRenderer, viewport[2], viewport[3]);
    }

    wasLMBPressed = isLMBPressed;
    wasRMBPressed = isRMBPressed;

    if (glUseProgramPtr) glUseProgramPtr(lastProgram);
    if (glActiveTexturePtr) glActiveTexturePtr(lastActiveTexture);
    glPopAttrib();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    env->DeleteLocalRef(fontRenderer);
    env->DeleteLocalRef(mc);
}
