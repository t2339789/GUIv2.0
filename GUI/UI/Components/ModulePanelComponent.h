#pragma once
#include "../Core/UIComponent.h"
#include "../Core/Canvas.h"
#include "../../ModuleManager.h"
#include <string>
#include <cstdio>

class ModulePanelComponent : public UIComponent {
public:
    void onUpdate() override {}

    void handleInput(int mouseX, int mouseY, bool lmbDown, bool clicked, bool released) {
        const bool titleHit = hit(mouseX, mouseY, panelX, panelY, panelW, titleH);
        if (clicked && titleHit) {
            dragging = true;
            dragOffsetX = mouseX - (int)panelX;
            dragOffsetY = mouseY - (int)panelY;
        }
        if (released) {
            dragging = false;
            activeSliderModule = -1;
            activeSliderSetting = -1;
        }
        if (dragging && lmbDown) {
            panelX = (float)(mouseX - dragOffsetX);
            panelY = (float)(mouseY - dragOffsetY);
        }
    }

    void onDraw(Canvas& c) override {
        auto& modules = ModuleManager::getModules();
        if (modules.empty()) return;
        if (selectedModule < 0 || selectedModule >= (int)modules.size()) selectedModule = 0;

        const float shellPad = 16.0f;
        const float navW = 190.0f;
        const float gap = 14.0f;
        const float navX = panelX + shellPad;
        const float navY = panelY + shellPad;
        const float navH = panelH - shellPad * 2.0f;
        const float bodyX = navX + navW + gap;
        const float bodyY = navY;
        const float bodyW = panelW - shellPad * 2.0f - navW - gap;
        const float bodyH = navH;
        const float accentR = 0.13f, accentG = 0.83f, accentB = 0.93f;

        c.drawRoundRect(panelX, panelY, panelW, panelH, 30.0f, 1.0f, 1.0f, 1.0f, 0.10f);
        c.drawRoundRect(panelX + 1.0f, panelY + 1.0f, panelW - 2.0f, panelH - 2.0f, 29.0f, 0.06f, 0.10f, 0.16f, 0.68f);
        c.drawRoundRect(panelX + 2.0f, panelY + 2.0f, panelW - 4.0f, panelH - 4.0f, 28.0f, 0.20f, 0.31f, 0.41f, 0.34f);
        c.drawRoundRect(panelX + 6.0f, panelY + 6.0f, panelW - 12.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.18f);
        c.drawRoundRect(panelX + 7.0f, panelY + 7.0f, 2.0f, panelH - 14.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.12f);

        drawGlassPanel(c, navX, navY, navW, navH, 24.0f);
        drawGlassPanel(c, bodyX, bodyY, bodyW, bodyH, 24.0f);

        drawHeader(c, navX, navY, accentR, accentG, accentB);
        drawNav(c, modules, navX, navY, navW, navH, accentR, accentG, accentB);
        drawDetails(c, modules[selectedModule], bodyX, bodyY, bodyW, bodyH, accentR, accentG, accentB);

        lastClick = false;
        lastRightClick = false;
    }

    void setFrameInput(int mouseX, int mouseY, bool lmbDown, bool click, bool rclick) {
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        lastLmbDown = lmbDown;
        lastClick = click;
        lastRightClick = rclick;
    }

private:
    float panelX = 40.0f;
    float panelY = 40.0f;
    float panelW = 560.0f;
    float panelH = 500.0f;
    float titleH = 44.0f;
    bool dragging = false;
    int dragOffsetX = 0;
    int dragOffsetY = 0;
    int selectedModule = 0;
    int activeSliderModule = -1;
    int activeSliderSetting = -1;

    int lastMouseX = 0;
    int lastMouseY = 0;
    bool lastLmbDown = false;
    bool lastClick = false;
    bool lastRightClick = false;

    static bool hit(int mx, int my, float x, float y, float w, float h) {
        return mx >= (int)x && mx <= (int)(x + w) && my >= (int)y && my <= (int)(y + h);
    }

    static std::string fixed2(float value) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", value);
        return std::string(buf);
    }

    static const char* moduleKey(const std::string& name) {
        if (name == "ESP") return "F1";
        if (name == "AimAssist") return "F3";
        if (name == "Velocity") return "F4";
        if (name == "StrafeAssist") return "F5";
        if (name == "DiscreetFly") return "F6";
        if (name == "HitReg") return "F7";
        if (name == "StatChecker") return "F8";
        if (name == "RotationSync") return "F9";
        if (name == "ComboBreaker") return "F10";
        return "";
    }

    static const char* sectionTitle(const std::string& name) {
        if (name == "ESP") return "Rendering";
        if (name == "AimAssist") return "Targeting";
        if (name == "Velocity") return "Movement";
        if (name == "DiscreetFly") return "Flight";
        if (name == "StrafeAssist") return "Positioning";
        return "Settings";
    }

    static const char* settingDesc(const std::string& name) {
        if (name == "Vines") return "Render hitbox vines";
        if (name == "Tracers") return "Connect cursor to hitbox center";
        if (name == "Tracer Radius") return "Max tracer distance";
        if (name == "Range") return "Max lock distance";
        if (name == "FOV") return "Aim cone angle";
        if (name == "Smoothing") return "Correction smoothing";
        if (name == "Strength") return "Correction intensity";
        if (name == "Team Check") return "Ignore teammates";
        if (name == "Require Click") return "Only while clicking";
        if (name == "NPCs") return "Include NPC targets";
        if (name == "Debug") return "Show diagnostics";
        if (name == "Horizontal") return "Horizontal knockback";
        if (name == "Vertical") return "Vertical knockback";
        if (name == "Jump Reset") return "Reset on jump";
        if (name == "Jump Chance") return "Reset probability";
        if (name == "Friction") return "Ground friction";
        if (name == "W-Tap") return "Auto tap movement";
        if (name == "Horizontal Speed") return "Forward/strafe speed";
        if (name == "Vertical Speed") return "Rise/fall speed";
        if (name == "Glide") return "Idle descent";
        if (name == "Anti Kick") return "Small downward pulse";
        if (name == "Require Input") return "Move only while pressing keys";
        if (name == "Target Distance") return "Preferred range";
        if (name == "Avoid Holes") return "Reverse near holes";
        return "";
    }

    void drawGlassPanel(Canvas& c, float x, float y, float w, float h, float radius) {
        c.drawRoundRect(x, y, w, h, radius, 0.89f, 0.96f, 1.0f, 0.20f);
        c.drawRoundRect(x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, radius - 1.0f, 0.17f, 0.31f, 0.39f, 0.48f);
        c.drawRoundRect(x + 2.0f, y + 2.0f, w - 4.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.22f);
        c.drawRoundRect(x + 2.0f, y + 2.0f, 2.0f, h - 4.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.16f);
    }

    void drawRaisedButton(Canvas& c, float x, float y, float w, float h, float radius, bool selected, bool enabled) {
        if (selected) {
            c.drawRoundRect(x, y + 2.0f, w, h - 2.0f, radius, 0.0f, 0.0f, 0.0f, 0.18f);
            c.drawRoundRect(x + 1.0f, y + 3.0f, w - 2.0f, h - 4.0f, radius - 1.0f, 0.02f, 0.05f, 0.08f, 0.78f);
            c.drawRoundRect(x + 7.0f, y + 8.0f, w - 14.0f, h - 16.0f, radius - 6.0f, 1.0f, 1.0f, 1.0f, 0.04f);
        } else {
            c.drawRoundRect(x, y, w, h, radius, 0.88f, 0.96f, 1.0f, 0.22f);
            c.drawRoundRect(x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f, radius - 1.0f, 0.35f, 0.50f, 0.60f, enabled ? 0.36f : 0.26f);
            c.drawRoundRect(x + 2.0f, y + 2.0f, w - 4.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.22f);
            c.drawRoundRect(x + 2.0f, y + 2.0f, 2.0f, h - 4.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.16f);
        }
    }

    void drawHeader(Canvas& c, float navX, float navY, float ar, float ag, float ab) {
        c.drawRoundRect(navX + 17.0f, navY + 13.0f, 24.0f, 24.0f, 9.0f, ar, ag, ab, 0.96f);
        c.drawRoundRect(navX + 23.0f, navY + 13.0f, 18.0f, 24.0f, 8.0f, 0.93f, 0.28f, 0.62f, 0.52f);
        c.drawTextSize("Poltergeist", navX + 51.0f, navY + 18.0f, 14.0f, 0.97f, 0.99f, 1.0f, 0.95f);
    }

    void drawNav(Canvas& c, std::vector<Module*>& modules, float navX, float navY, float navW, float navH, float ar, float ag, float ab) {
        const float rowX = navX + 15.0f;
        const float rowW = navW - 30.0f;
        const float statH = 55.0f;
        const float statY = navY + navH - 13.0f - statH;
        const float rowGap = 7.0f;
        const int count = (int)modules.size();
        float rowH = count > 0 ? (statY - (navY + 58.0f) - rowGap * (count - 1)) / count : 40.0f;
        if (rowH > 56.0f) rowH = 56.0f;
        if (rowH < 34.0f) rowH = 34.0f;
        float y = navY + 58.0f;
        for (int i = 0; i < (int)modules.size(); ++i) {
            Module* mod = modules[i];
            const bool selected = (i == selectedModule);
            const bool enabled = mod->isEnabled();
            const float toggleX = rowX + rowW - 43.0f;
            const float toggleY = y + (rowH - 18.0f) * 0.5f;

            drawRaisedButton(c, rowX, y, rowW, rowH, 18.0f, selected, enabled);
            if (hit(lastMouseX, lastMouseY, rowX, y, rowW, rowH)) {
                c.drawRoundRect(rowX, y, rowW, rowH, 18.0f, 1.0f, 1.0f, 1.0f, 0.08f);
            }

            if (selected) {
                c.drawTextSize(mod->getName(), rowX + 12.0f, y + 8.0f, 13.0f, ar, ag, ab, 1.0f);
            } else {
                const float textA = enabled ? 0.96f : 0.74f;
                c.drawTextSize(mod->getName(), rowX + 12.0f, y + 8.0f, 13.0f, textA, 0.98f, 1.0f, 1.0f);
            }
            if (rowH >= 44.0f) {
                c.drawTextSize(moduleKey(mod->getName()), rowX + 12.0f, y + 27.0f, 10.0f, 0.72f, 0.80f, 0.90f, 0.72f);
            }
            drawSwitch(c, toggleX, toggleY, enabled, ar, ag, ab);

            const bool toggleHit = hit(lastMouseX, lastMouseY, toggleX - 7.0f, toggleY - 7.0f, 46.0f, 32.0f);
            if (lastClick && toggleHit) {
                mod->toggle();
            } else if (lastClick && hit(lastMouseX, lastMouseY, rowX, y, rowW - 40.0f, rowH)) {
                selectedModule = i;
            }
            y += rowH + rowGap;
        }

        int active = 0;
        for (Module* mod : modules) if (mod->isEnabled()) active++;
        c.drawRoundRect(rowX, statY, rowW, statH, 18.0f, 0.0f, 0.0f, 0.0f, 0.16f);
        c.drawRoundRect(rowX + 5.0f, statY + 5.0f, rowW - 10.0f, statH - 10.0f, 14.0f, 1.0f, 1.0f, 1.0f, 0.04f);
        c.drawTextSize(std::to_string(active) + " / " + std::to_string((int)modules.size()) + " active", rowX + 13.0f, statY + 10.0f, 10.0f, 0.78f, 0.88f, 0.96f, 0.86f);
        float pct = modules.empty() ? 0.0f : (float)active / (float)modules.size();
        c.drawRoundRect(rowX + 13.0f, statY + 37.0f, rowW - 26.0f, 5.0f, 3.0f, 1.0f, 1.0f, 1.0f, 0.18f);
        c.drawRoundRect(rowX + 13.0f, statY + 37.0f, (rowW - 26.0f) * pct, 5.0f, 3.0f, ar, ag, ab, 0.95f);
        c.drawRoundRect(rowX + 13.0f, statY + 37.0f, (rowW - 26.0f) * pct, 2.0f, 1.0f, 0.96f, 0.55f, 0.75f, 0.62f);
    }

    void drawDetails(Canvas& c, Module* mod, float bodyX, float bodyY, float bodyW, float bodyH, float ar, float ag, float ab) {
        c.drawTextSize(mod->getName(), bodyX + 20.0f, bodyY + 15.0f, 21.0f, 0.96f, 0.99f, 1.0f, 1.0f);
        const float badgeX = bodyX + bodyW - 93.0f;
        if (mod->isEnabled()) {
            c.drawRoundRect(badgeX, bodyY + 16.0f, 73.0f, 24.0f, 12.0f, ar, ag, ab, 0.14f);
            c.drawRoundRect(badgeX + 1.0f, bodyY + 17.0f, 71.0f, 22.0f, 11.0f, 1.0f, 1.0f, 1.0f, 0.04f);
            c.drawTextSize("ENABLED", badgeX + 10.0f, bodyY + 22.0f, 10.0f, ar, ag, ab, 1.0f);
        } else {
            c.drawRoundRect(badgeX - 4.0f, bodyY + 16.0f, 77.0f, 24.0f, 12.0f, 1.0f, 1.0f, 1.0f, 0.08f);
            c.drawTextSize("DISABLED", badgeX + 2.0f, bodyY + 22.0f, 10.0f, 0.72f, 0.80f, 0.88f, 1.0f);
        }
        c.drawRect(bodyX + 1.0f, bodyY + 62.0f, bodyW - 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.10f);

        auto& settings = mod->getSettings();
        const float bodyPad = 16.0f;
        const float sectionX = bodyX + bodyPad;
        const float sectionY = bodyY + 78.0f;
        const float sectionW = bodyW - bodyPad * 2.0f;
        const float sectionH = bodyH - 94.0f;
        c.drawRoundRect(sectionX, sectionY, sectionW, sectionH, 20.0f, 1.0f, 1.0f, 1.0f, 0.08f);
        c.drawRoundRect(sectionX + 1.0f, sectionY + 1.0f, sectionW - 2.0f, sectionH - 2.0f, 19.0f, 0.45f, 0.55f, 0.68f, 0.14f);
        c.drawRoundRect(sectionX + 2.0f, sectionY + 2.0f, sectionW - 4.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.14f);
        c.drawTextSize(sectionTitle(mod->getName()), sectionX + 14.0f, sectionY + 9.0f, 11.0f, 0.78f, 0.88f, 0.96f, 0.86f);
        float y = sectionY + 34.0f;

        for (int si = 0; si < (int)settings.size(); ++si) {
            Setting& s = settings[si];
            const float rowH = 45.0f;
            const float rowX = sectionX + 12.0f;
            const float rowW = sectionW - 24.0f;
            if (hit(lastMouseX, lastMouseY, rowX, y, rowW, rowH)) {
                c.drawRoundRect(rowX, y + 2.0f, rowW, rowH - 4.0f, 15.0f, 1.0f, 1.0f, 1.0f, 0.07f);
            }
            c.drawTextSize(s.name, rowX + 7.0f, y + 7.0f, 12.0f, 0.95f, 0.98f, 1.0f, 0.92f);
            c.drawTextSize(settingDesc(s.name), rowX + 7.0f, y + 25.0f, 9.0f, 0.72f, 0.80f, 0.90f, 0.66f);

            if (s.type == Setting::BOOL_TOGGLE) {
                const float tx = sectionX + sectionW - 47.0f;
                const float ty = y + 12.0f;
                drawSwitch(c, tx, ty, s.getBool(), ar, ag, ab);
                if (lastClick && hit(lastMouseX, lastMouseY, tx - 8.0f, ty - 9.0f, 44.0f, 30.0f)) {
                    s.setBool(!s.getBool());
                }
            } else {
                const float valueX = sectionX + sectionW - 46.0f;
                const float sliderX = rowX + 126.0f;
                const float sliderY = y + 21.0f;
                const float sliderW = valueX - sliderX - 8.0f;
                float pct = (s.value - s.minVal) / (s.maxVal - s.minVal);
                if (pct < 0.0f) pct = 0.0f;
                if (pct > 1.0f) pct = 1.0f;
                c.drawRoundRect(sliderX, sliderY, sliderW, 4.0f, 2.0f, 1.0f, 1.0f, 1.0f, 0.20f);
                c.drawRoundRect(sliderX, sliderY, sliderW * pct, 4.0f, 2.0f, ar, ag, ab, 0.95f);
                c.drawRoundRect(sliderX, sliderY, sliderW * pct, 2.0f, 1.0f, 0.96f, 0.55f, 0.75f, 0.62f);
                c.drawRoundRect(sliderX + sliderW * pct - 7.0f, sliderY - 5.0f, 14.0f, 14.0f, 7.0f, 0.96f, 0.99f, 1.0f, 1.0f);
                c.drawRoundRect(sliderX + sliderW * pct - 3.0f, sliderY - 1.0f, 6.0f, 6.0f, 3.0f, ar, ag, ab, 0.42f);
                c.drawTextSize(fixed2(s.value), valueX, y + 14.0f, 10.0f, ar, ag, ab, 1.0f);

                const bool sliderHit = hit(lastMouseX, lastMouseY, sliderX, sliderY - 9.0f, sliderW, 20.0f);
                if (lastClick && sliderHit) {
                    activeSliderModule = selectedModule;
                    activeSliderSetting = si;
                }
                if (lastLmbDown && activeSliderModule == selectedModule && activeSliderSetting == si) {
                    float newPct = ((float)lastMouseX - sliderX) / sliderW;
                    if (newPct < 0.0f) newPct = 0.0f;
                    if (newPct > 1.0f) newPct = 1.0f;
                    s.value = s.minVal + newPct * (s.maxVal - s.minVal);
                }
            }
            y += rowH;
            if (y > bodyY + bodyH - 20.0f) break;
        }
    }

    void drawSwitch(Canvas& c, float x, float y, bool on, float ar, float ag, float ab) {
        const float trackW = 32.0f;
        const float trackH = 18.0f;
        if (on) {
            c.drawRoundRect(x, y, trackW, trackH, 9.0f, ar, ag, ab, 0.32f);
            c.drawRoundRect(x + 1.0f, y + 1.0f, trackW - 2.0f, trackH - 2.0f, 8.0f, ar, ag, ab, 0.78f);
        } else {
            c.drawRoundRect(x, y, trackW, trackH, 9.0f, 0.72f, 0.80f, 0.90f, 0.18f);
            c.drawRoundRect(x + 1.0f, y + 1.0f, trackW - 2.0f, trackH - 2.0f, 8.0f, 0.13f, 0.17f, 0.23f, 0.70f);
        }
        c.drawRoundRect(x + 2.0f, y + 2.0f, trackW - 4.0f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f, on ? 0.24f : 0.10f);
        const float knobX = on ? x + 17.0f : x + 3.0f;
        c.drawRoundRect(knobX, y + 3.0f, 12.0f, 12.0f, 6.0f, on ? 0.94f : 0.70f, on ? 1.0f : 0.76f, on ? 0.98f : 0.86f, 1.0f);
        if (on) c.drawRoundRect(knobX + 3.0f, y + 6.0f, 6.0f, 6.0f, 3.0f, ar, ag, ab, 0.70f);
    }
};
