#include "AimAssistModule.h"
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern JavaVM* g_vm;
extern bool g_initialized;

extern jclass mcClass, entityClass, playerClass;

extern jfieldID mcInstanceField;
extern jfieldID localPlayerField;
extern jfieldID worldField;
extern jfieldID playersListField;
extern jfieldID entityListField;

extern jfieldID posXField;
extern jfieldID posYField;
extern jfieldID posZField;
extern jfieldID rotationYawField;
extern jfieldID rotationPitchField;
extern jfieldID isDeadField;
extern jfieldID widthField;
extern jfieldID heightField;

extern jmethodID toArrayMethod;
extern jmethodID isInvisibleMethod;
extern jmethodID getTeamMethod;
extern jmethodID isSameTeamMethod;

static float wrapAngleTo180(float angle) {
    while (angle <= -180.0f) angle += 360.0f;
    while (angle > 180.0f) angle -= 360.0f;
    return angle;
}

static void debugPrint(bool enabled, const char* msg) {
    if (!enabled || !msg) return;
    OutputDebugStringA("[AimAssist] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

AimAssistModule::AimAssistModule()
    : Module("AimAssist", false),
      wasLMBPressed(false),
      lastYaw(0.0f),
      lastPitch(0.0f),
      smoothedYaw(0.0f),
      smoothedPitch(0.0f)
{
    addSetting("Range", 4.0f, 1.0f, 6.0f);
    addSetting("FOV", 25.0f, 5.0f, 90.0f);
    addSetting("Smoothing", 60.0f, 10.0f, 100.0f);
    addSetting("Strength", 0.5f, 0.1f, 1.0f);

    addSetting("Team Check", false);

    // Old working version always required click/CPS.
    // This keeps that behavior by default, but still lets you disable it in the GUI.
    addSetting("Require Click", true);

    // Kept, but now safely falls back to playersListField if entityListField is missing.
    addSetting("NPCs", false);

    addSetting("Debug", false);
}

void AimAssistModule::onRender2D() {
    if (!enabled || !g_vm || !g_initialized) return;

    Setting* debugSetting = getSetting("Debug");
    bool debug = debugSetting && debugSetting->getBool();

    unsigned long long now = GetTickCount64();

    // --- CPS tracking, same style as old working version ---
    bool isLMB = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

    if (isLMB && !wasLMBPressed) {
        clicks.push_back(now);
    }

    wasLMBPressed = isLMB;

    clicks.erase(
        std::remove_if(
            clicks.begin(),
            clicks.end(),
            [now](unsigned long long t) {
                return now - t > 1000;
            }
        ),
        clicks.end()
    );

    Setting* requireClickSetting = getSetting("Require Click");
    bool requireClick = requireClickSetting && requireClickSetting->getBool();

    if (requireClick && clicks.size() < 4) {
        debugPrint(debug, "return: Require Click enabled and CPS < 4");
        return;
    }

    JNIEnv* env = nullptr;

    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK || !env) {
        debugPrint(debug, "return: no JNIEnv");
        return;
    }

    jobject mc = env->GetStaticObjectField(mcClass, mcInstanceField);
    if (!mc) {
        debugPrint(debug, "return: mc null");
        return;
    }

    jobject localPlayer = env->GetObjectField(mc, localPlayerField);
    jobject world = env->GetObjectField(mc, worldField);

    if (!localPlayer || !world) {
        debugPrint(debug, "return: localPlayer/world null");

        if (localPlayer) env->DeleteLocalRef(localPlayer);
        if (world) env->DeleteLocalRef(world);
        env->DeleteLocalRef(mc);
        return;
    }

    float lpYaw = env->GetFloatField(localPlayer, rotationYawField);
    float lpPitch = env->GetFloatField(localPlayer, rotationPitchField);

    float userYawDelta = (lastYaw == 0.0f) ? 0.0f : wrapAngleTo180(lpYaw - lastYaw);
    float userPitchDelta = (lastPitch == 0.0f) ? 0.0f : (lpPitch - lastPitch);

    // Old working behavior:
    // only assist if the user is actively moving the mouse.
    if (std::abs(userYawDelta) < 0.001f && std::abs(userPitchDelta) < 0.001f) {
        lastYaw = lpYaw;
        lastPitch = lpPitch;

        debugPrint(debug, "return: no user mouse movement");

        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(mc);
        return;
    }

    jobject lpTeam = nullptr;

    if (getTeamMethod) {
        lpTeam = env->CallObjectMethod(localPlayer, getTeamMethod);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            lpTeam = nullptr;
        }
    }

    double lpX = env->GetDoubleField(localPlayer, posXField);
    double lpY = env->GetDoubleField(localPlayer, posYField);
    double lpZ = env->GetDoubleField(localPlayer, posZField);
    double lpEyeY = lpY + 1.62;

    Setting* npcsSetting = getSetting("NPCs");
    bool useNPCs = npcsSetting && npcsSetting->getBool();

    // New-feature-safe list selection:
    // If NPCs is on but entityListField is missing, fall back to playersListField.
    jfieldID listField = playersListField;

    if (useNPCs) {
        if (entityListField) {
            listField = entityListField;
        } else {
            debugPrint(debug, "NPCs enabled but entityListField is null; falling back to playersListField");
            listField = playersListField;
        }
    }

    if (!listField) {
        debugPrint(debug, "return: no usable listField");

        if (lpTeam) env->DeleteLocalRef(lpTeam);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(mc);
        return;
    }

    jobject entityList = env->GetObjectField(world, listField);

    if (!entityList) {
        debugPrint(debug, "return: entity/player list null");

        if (lpTeam) env->DeleteLocalRef(lpTeam);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(mc);
        return;
    }

    jobjectArray entities = (jobjectArray)env->CallObjectMethod(entityList, toArrayMethod);

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        entities = nullptr;
    }

    if (!entities) {
        debugPrint(debug, "return: toArray returned null");

        env->DeleteLocalRef(entityList);
        if (lpTeam) env->DeleteLocalRef(lpTeam);
        env->DeleteLocalRef(localPlayer);
        env->DeleteLocalRef(world);
        env->DeleteLocalRef(mc);
        return;
    }

    int count = env->GetArrayLength(entities);

    jobject bestTarget = nullptr;
    double targetX = 0.0;
    double targetY = 0.0;
    double targetZ = 0.0;

    Setting* fovSetting = getSetting("FOV");
    Setting* rangeSetting = getSetting("Range");
    Setting* teamCheckSetting = getSetting("Team Check");

    float fov = fovSetting ? fovSetting->value : 25.0f;
    float range = rangeSetting ? rangeSetting->value : 4.0f;
    bool teamCheck = teamCheckSetting && teamCheckSetting->getBool();

    float bestDiff = fov;

    for (int i = 0; i < count; i++) {
        jobject entity = env->GetObjectArrayElement(entities, i);
        if (!entity) continue;

        if (env->IsSameObject(entity, localPlayer)) {
            env->DeleteLocalRef(entity);
            continue;
        }

        // If NPCs is off, only allow playerClass.
        // If NPCs is on, allow entityClass/playerClass objects, but still skip invalid entities.
        if (!useNPCs && playerClass && !env->IsInstanceOf(entity, playerClass)) {
            env->DeleteLocalRef(entity);
            continue;
        }

        if (useNPCs && entityClass && !env->IsInstanceOf(entity, entityClass)) {
            env->DeleteLocalRef(entity);
            continue;
        }

        if (isDeadField && env->GetBooleanField(entity, isDeadField)) {
            env->DeleteLocalRef(entity);
            continue;
        }

        if (isInvisibleMethod) {
            jboolean invisible = env->CallBooleanMethod(entity, isInvisibleMethod);

            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                invisible = false;
            }

            if (invisible) {
                env->DeleteLocalRef(entity);
                continue;
            }
        }

        // Team check only makes sense for player-like entities.
        if (teamCheck && lpTeam && getTeamMethod && isSameTeamMethod) {
            jobject team = env->CallObjectMethod(entity, getTeamMethod);

            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                team = nullptr;
            }

            if (team) {
                jboolean sameTeam = env->CallBooleanMethod(team, isSameTeamMethod, lpTeam);

                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    sameTeam = false;
                }

                env->DeleteLocalRef(team);

                if (sameTeam) {
                    env->DeleteLocalRef(entity);
                    continue;
                }
            }
        }

        double eX = env->GetDoubleField(entity, posXField);
        double eY = env->GetDoubleField(entity, posYField);
        double eZ = env->GetDoubleField(entity, posZField);

        float eH = 1.8f;

        if (heightField) {
            eH = env->GetFloatField(entity, heightField);
        }

        // Same target point as old working version: upper chest.
        double targetX_e = eX;
        double targetY_e = eY + (eH * 0.45);
        double targetZ_e = eZ;

        double dx = targetX_e - lpX;
        double dy = targetY_e - lpEyeY;
        double dz = targetZ_e - lpZ;

        double dist = sqrt(dx * dx + dy * dy + dz * dz);

        if (dist > range) {
            env->DeleteLocalRef(entity);
            continue;
        }

        float yawToTarget = (float)(atan2(dz, dx) * 180.0 / M_PI) - 90.0f;
        float diff = wrapAngleTo180(yawToTarget - lpYaw);

        if (std::abs(diff) < bestDiff) {
            bestDiff = std::abs(diff);

            if (bestTarget) {
                env->DeleteLocalRef(bestTarget);
            }

            bestTarget = entity;
            targetX = targetX_e;
            targetY = targetY_e;
            targetZ = targetZ_e;
        } else {
            env->DeleteLocalRef(entity);
        }
    }

    if (bestTarget) {
        double dx = targetX - lpX;
        double dy = targetY - lpEyeY;
        double dz = targetZ - lpZ;

        float targetYaw = (float)(atan2(dz, dx) * 180.0 / M_PI) - 90.0f;
        float targetPitch = (float)-(atan2(dy, sqrt(dx * dx + dz * dz)) * 180.0 / M_PI);

        // Same EMA style as old working version.
        float alpha = 0.15f;

        if (std::abs(smoothedYaw) < 0.01f) {
            smoothedYaw = targetYaw;
            smoothedPitch = targetPitch;
        }

        float yawGap = wrapAngleTo180(targetYaw - smoothedYaw);
        smoothedYaw += alpha * yawGap;
        smoothedPitch = (alpha * targetPitch) + ((1.0f - alpha) * smoothedPitch);

        float yawDiff = wrapAngleTo180(smoothedYaw - lpYaw);
        float pitchDiff = smoothedPitch - lpPitch;

        Setting* smoothingSetting = getSetting("Smoothing");
        Setting* strengthSetting = getSetting("Strength");

        float smoothing = smoothingSetting ? smoothingSetting->value : 60.0f;
        float strengthMult = strengthSetting ? strengthSetting->value : 0.5f;

        // Same base as old working version, now multiplied by the new Strength feature.
        float currentStrength = (1.0f / (smoothing * 1.5f)) * strengthMult;

        float deltaYaw = yawDiff * currentStrength;
        float deltaPitch = pitchDiff * currentStrength;

        // Same resistance logic as old working version.
        // If the user is moving away from the assist, weaken it.
        if (userYawDelta * yawDiff < 0.0f) {
            deltaYaw *= 0.1f;
        }

        if (userPitchDelta * pitchDiff < 0.0f) {
            deltaPitch *= 0.1f;
        }

        float maxCurrentYaw = 0.08f * strengthMult;
        float maxCurrentPitch = 0.05f * strengthMult;

        if (std::abs(yawDiff) > 0.2f) {
            deltaYaw = std::clamp(deltaYaw, -maxCurrentYaw, maxCurrentYaw);
            lpYaw += deltaYaw;
        }

        if (std::abs(pitchDiff) > 0.2f) {
            deltaPitch = std::clamp(deltaPitch, -maxCurrentPitch, maxCurrentPitch);
            lpPitch += deltaPitch;
        }

        env->SetFloatField(localPlayer, rotationYawField, lpYaw);
        env->SetFloatField(localPlayer, rotationPitchField, lpPitch);

        lastYaw = lpYaw;
        lastPitch = lpPitch;

        env->DeleteLocalRef(bestTarget);

        debugPrint(debug, "applied aim assist");
    } else {
        lastYaw = lpYaw;
        lastPitch = lpPitch;

        smoothedYaw = 0.0f;
        smoothedPitch = 0.0f;

        debugPrint(debug, "no target");
    }

    if (lpTeam) env->DeleteLocalRef(lpTeam);

    env->DeleteLocalRef(entities);
    env->DeleteLocalRef(entityList);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
}
