#include "DiscreetFlyModule.h"
#include <windows.h>
#include <jni.h>
#include <cmath>

extern JavaVM* g_vm;
extern bool g_initialized;

extern jclass mcClass;
extern jfieldID mcInstanceField;
extern jfieldID localPlayerField;
extern jfieldID rotationYawField;
extern jfieldID motionXField;
extern jfieldID motionYField;
extern jfieldID motionZField;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DiscreetFlyModule::DiscreetFlyModule()
    : Module("DiscreetFly", false), lastAntiKickPulse(0) {
    addSetting("Horizontal Speed", 0.16f, 0.02f, 0.60f);
    addSetting("Vertical Speed", 0.14f, 0.02f, 0.60f);
    addSetting("Glide", 0.03f, 0.0f, 0.12f);
    addSetting("Anti Kick", true);
    addSetting("Require Input", true);
}

void DiscreetFlyModule::onRender2D() {
    if (!enabled || !g_vm || !g_initialized) return;

    JNIEnv* env = nullptr;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK || !env) return;

    jobject mc = env->GetStaticObjectField(mcClass, mcInstanceField);
    if (!mc) return;

    jobject localPlayer = env->GetObjectField(mc, localPlayerField);
    if (!localPlayer) {
        env->DeleteLocalRef(mc);
        return;
    }

    const bool forward = (GetAsyncKeyState('W') & 0x8000) != 0;
    const bool back = (GetAsyncKeyState('S') & 0x8000) != 0;
    const bool left = (GetAsyncKeyState('A') & 0x8000) != 0;
    const bool right = (GetAsyncKeyState('D') & 0x8000) != 0;
    const bool up = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    const bool down = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool anyInput = forward || back || left || right || up || down;

    Setting* requireInputSetting = getSetting("Require Input");
    bool requireInput = requireInputSetting && requireInputSetting->getBool();
    if (requireInput && !anyInput) {
        env->SetDoubleField(localPlayer, motionXField, 0.0);
        env->SetDoubleField(localPlayer, motionYField, -getSetting("Glide")->value);
        env->SetDoubleField(localPlayer, motionZField, 0.0);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(mc);
        return;
    }

    float yaw = env->GetFloatField(localPlayer, rotationYawField);
    float yawRad = yaw * (float)M_PI / 180.0f;
    float speed = getSetting("Horizontal Speed")->value;
    float verticalSpeed = getSetting("Vertical Speed")->value;
    float glide = getSetting("Glide")->value;

    float moveForward = 0.0f;
    float moveStrafe = 0.0f;
    if (forward) moveForward += 1.0f;
    if (back) moveForward -= 1.0f;
    if (right) moveStrafe += 1.0f;
    if (left) moveStrafe -= 1.0f;

    float len = sqrtf(moveForward * moveForward + moveStrafe * moveStrafe);
    if (len > 0.001f) {
        moveForward /= len;
        moveStrafe /= len;
    }

    double mx = (-sinf(yawRad) * moveForward + cosf(yawRad) * moveStrafe) * speed;
    double mz = (cosf(yawRad) * moveForward + sinf(yawRad) * moveStrafe) * speed;

    double my = -glide;
    if (up && !down) my = verticalSpeed;
    else if (down && !up) my = -verticalSpeed;

    Setting* antiKickSetting = getSetting("Anti Kick");
    bool antiKick = antiKickSetting && antiKickSetting->getBool();
    unsigned long long now = GetTickCount64();
    if (antiKick && !up && !down && now - lastAntiKickPulse > 1200) {
        my = -0.08;
        lastAntiKickPulse = now;
    }

    env->SetDoubleField(localPlayer, motionXField, mx);
    env->SetDoubleField(localPlayer, motionYField, my);
    env->SetDoubleField(localPlayer, motionZField, mz);

    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
}
