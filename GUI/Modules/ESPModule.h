#pragma once
#include "../Module.h"
#include <GL/gl.h>
#include <map>
#include <string>
#include <jni.h>

struct HealthHudLayout {
    float frameW = 120.0f;
    float frameH = 379.0f;
    float frameYShift = 0.0f;
    float heartW = 52.0f;
    float heartH = 30.0f;
    float heartSpacing = 0.079f;
    float heartXOffset = 0.020f;
    float heartYStart = 0.212f;
    int heartCount = 7;
    float billboardScale = 0.003166f;
    float sideOffset = 0.35f;
    float layerNudge = 0.0025f;
    float animFps = 10.0f;
};

struct PlayerState {
    float displayedHealth = -1.0f;
    float previousHealth = -1.0f;
    float healthVelocity = 0.0f;
    float impactPulse = 0.0f;
    float healPulse = 0.0f;
    float heartImpacts[60] = {};
    double animationTime = 0.0;
};

class ESPModule : public Module {
public:
    ESPModule();

    void onRender() override;
    void onDisable() override;

private:
    GLuint atlasTexture = 0;
    bool textureLoaded = false;

    static const int HUD_ANIM_FRAME_COUNT = 6;
    static const int HUD_HEART_VARIANT_COUNT = 3;
    GLuint hudAnimationTextures[HUD_ANIM_FRAME_COUNT] = {};
    bool hudAnimationTexturesLoaded[HUD_ANIM_FRAME_COUNT] = {};
    int hudAnimationTextureWidths[HUD_ANIM_FRAME_COUNT] = {};
    int hudAnimationTextureHeights[HUD_ANIM_FRAME_COUNT] = {};

    GLuint hudHeartTextures[HUD_ANIM_FRAME_COUNT][HUD_HEART_VARIANT_COUNT] = {};
    bool hudHeartTexturesLoaded[HUD_ANIM_FRAME_COUNT][HUD_HEART_VARIANT_COUNT] = {};

    HealthHudLayout hudLayout;
    void refreshHealthHudLayout();

    void loadDungeonTexture();
    void unloadTextures();

    double getAnimationTimeSeconds() const;
    std::string getPlayerName(JNIEnv* env, jobject player);

    std::map<std::string, PlayerState> playerStates;
    double lastRenderTime = 0.0;

    void drawAnimatedHealthHudBillboard(
        float x,
        float y,
        float z,
        float w,
        float h,
        float rightX,
        float rightZ
    );
};
