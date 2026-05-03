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
int ClickGUI::windowWidth = 220;
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
    if (glActiveTexturePtr) glActiveTexturePtr(0x84C0); // GL_TEXTURE0

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
    bool isZeroPressed = (GetAsyncKeyState(0x30) & 0x8000); // '0' key
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
    
    // Title bar dragging
    if (clicked && isVisible) {
        if (mouseX >= windowX && mouseX <= windowX + windowWidth &&
            mouseY >= windowY && mouseY <= windowY + 25) {
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
        // Calculate dynamic window height
        std::vector<Module*>& modules = ModuleManager::getModules();
        int totalHeight = 35; // Header + padding
        for (int i = 0; i < (int)modules.size(); i++) {
            totalHeight += 22; // Module row
            if (expandedModule == i) {
                totalHeight += (int)modules[i]->getSettings().size() * 22 + 8;
            }
        }
        windowHeight = totalHeight;

        // Main window background - dark with subtle border
        drawRect((float)windowX - 1, (float)windowY - 1, (float)windowWidth + 2, (float)windowHeight + 2, 0.15f, 0.15f, 0.4f, 0.6f); // Border glow
        drawRect((float)windowX, (float)windowY, (float)windowWidth, (float)windowHeight, 0.08f, 0.08f, 0.08f, 0.95f);
        
        // Header bar - gradient feel
        drawRect((float)windowX, (float)windowY, (float)windowWidth, 25.0f, 0.12f, 0.12f, 0.35f, 1.0f);
        drawRect((float)windowX, (float)windowY + 23, (float)windowWidth, 2.0f, 0.3f, 0.3f, 0.8f, 0.8f); // Accent line
        
        drawText(env, fontRenderer, "§l§9Poltergeist", windowX + 8, windowY + 7);

        int startY = windowY + 32;
        for (int i = 0; i < (int)modules.size(); i++) {
            Module* mod = modules[i];
            bool hover = (mouseX >= windowX + 5 && mouseX <= windowX + windowWidth - 5 &&
                          mouseY >= startY && mouseY <= startY + 20);
            
            // Left-click: toggle module
            if (clicked && hover) {
                mod->toggle();
            }
            // Right-click: expand/collapse settings
            if (rightClicked && hover && !mod->getSettings().empty()) {
                expandedModule = (expandedModule == i) ? -1 : i;
            }

            // Module row background
            if (mod->isEnabled()) {
                drawRect((float)windowX + 4, (float)startY, (float)windowWidth - 8, 20.0f, 0.15f, 0.55f, 0.25f, 0.7f);
            } else {
                drawRect((float)windowX + 4, (float)startY, (float)windowWidth - 8, 20.0f, 0.12f, 0.12f, 0.12f, 0.7f);
            }
            
            // Hover highlight
            if (hover) {
                drawRect((float)windowX + 4, (float)startY, (float)windowWidth - 8, 20.0f, 1.0f, 1.0f, 1.0f, 0.1f);
            }

            // Module name
            std::string label = mod->isEnabled() ? ("§a" + mod->getName()) : ("§7" + mod->getName());
            drawText(env, fontRenderer, label.c_str(), windowX + 12, startY + 5);

            // Settings indicator arrow
            if (!mod->getSettings().empty()) {
                const char* arrow = (expandedModule == i) ? "§8v" : "§8>";
                drawText(env, fontRenderer, arrow, windowX + windowWidth - 18, startY + 5);
            }
            
            startY += 22;

            // --- Settings Panel (expanded) ---
            if (expandedModule == i) {
                drawRect((float)windowX + 8, (float)startY, (float)windowWidth - 16, (float)(mod->getSettings().size() * 22 + 4), 0.05f, 0.05f, 0.05f, 0.9f);
                startY += 4;

                auto& settings = mod->getSettings();
                for (int j = 0; j < (int)settings.size(); j++) {
                    Setting& s = settings[j];
                    
                    if (s.type == Setting::FLOAT_SLIDER) {
                        // Setting label + value
                        std::stringstream ss;
                        ss << "§7" << s.name << ": §f" << std::fixed << std::setprecision(2) << s.value;
                        drawText(env, fontRenderer, ss.str().c_str(), windowX + 14, startY + 2);

                        // Slider track
                        float sliderX = (float)windowX + 14;
                        float sliderW = (float)windowWidth - 32;
                        float sliderY = (float)startY + 14;
                        float sliderH = 4.0f;
                        
                        drawRect(sliderX, sliderY, sliderW, sliderH, 0.2f, 0.2f, 0.2f, 1.0f); // Track bg

                        // Filled portion
                        float pct = (s.value - s.minVal) / (s.maxVal - s.minVal);
                        drawRect(sliderX, sliderY, sliderW * pct, sliderH, 0.3f, 0.5f, 0.9f, 1.0f); // Fill

                        // Knob
                        float knobX = sliderX + sliderW * pct - 3;
                        drawRect(knobX, sliderY - 2, 6, 8, 0.9f, 0.9f, 0.9f, 1.0f);

                        // Slider interaction
                        bool sliderHover = (mouseX >= (int)sliderX && mouseX <= (int)(sliderX + sliderW) &&
                                           mouseY >= (int)(sliderY - 4) && mouseY <= (int)(sliderY + sliderH + 4));
                        
                        if (clicked && sliderHover) {
                            activeSlider = j + i * 100; // Unique ID
                        }
                        if (isLMBPressed && activeSlider == j + i * 100) {
                            float newPct = ((float)mouseX - sliderX) / sliderW;
                            if (newPct < 0) newPct = 0;
                            if (newPct > 1) newPct = 1;
                            s.value = s.minVal + newPct * (s.maxVal - s.minVal);
                        }

                    } else if (s.type == Setting::BOOL_TOGGLE) {
                        // Toggle box
                        float boxX = (float)windowX + 14;
                        float boxY = (float)startY + 3;
                        
                        if (s.getBool()) {
                            drawRect(boxX, boxY, 12, 12, 0.3f, 0.7f, 0.3f, 1.0f);
                        } else {
                            drawRect(boxX, boxY, 12, 12, 0.25f, 0.25f, 0.25f, 1.0f);
                        }
                        drawText(env, fontRenderer, ("§7" + s.name).c_str(), windowX + 32, startY + 4);

                        bool toggleHover = (mouseX >= (int)boxX && mouseX <= (int)(boxX + 12) &&
                                           mouseY >= (int)boxY && mouseY <= (int)(boxY + 12));
                        if (clicked && toggleHover) {
                            s.setBool(!s.getBool());
                        }
                    }
                    
                    startY += 22;
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
