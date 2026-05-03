#include "StrafeAssistModule.h"
#include <windows.h>
#include <jni.h>
#include <cmath>
#include <algorithm>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern JavaVM* g_vm;
extern bool g_initialized;
extern jclass mcClass, worldClass, playerClass, blockPosClass, movementInputClass;
extern jfieldID mcInstanceField, localPlayerField, worldField, playersListField, posXField, posYField, posZField, rotationYawField, isDeadField, onGroundField;
extern jfieldID movementInputInstanceField, moveForwardField, moveStrafeField;
extern jmethodID toArrayMethod, isAirBlockMethod, blockPosConstructor;

StrafeAssistModule::StrafeAssistModule() : Module("StrafeAssist", false), lastStrafeDir(1.0f), lastSwitchTime(0) {
    addSetting("Target Distance", 3.0f, 2.0f, 5.0f);
    addSetting("Strength", 0.5f, 0.1f, 1.0f);
    addSetting("Avoid Holes", true);
}

void StrafeAssistModule::onRender2D() {
    if (!enabled || !g_vm || !g_initialized) return;

    JNIEnv* env = nullptr;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK || !env) return;

    jobject mc = env->GetStaticObjectField(mcClass, mcInstanceField);
    if (!mc) return;

    jobject localPlayer = env->GetObjectField(mc, localPlayerField);
    jobject world = env->GetObjectField(mc, worldField);
    if (!localPlayer || !world) { env->DeleteLocalRef(mc); return; }

    unsigned long long now = GetTickCount64();
    double lpX = env->GetDoubleField(localPlayer, posXField);
    double lpY = env->GetDoubleField(localPlayer, posYField);
    double lpZ = env->GetDoubleField(localPlayer, posZField);
    float lpYaw = env->GetFloatField(localPlayer, rotationYawField);

    // Find nearest target
    jobject playersList = env->GetObjectField(world, playersListField);
    jobjectArray players = (jobjectArray)env->CallObjectMethod(playersList, toArrayMethod);
    int count = env->GetArrayLength(players);

    jobject bestTarget = nullptr;
    double bestDist = 6.0;

    for (int i = 0; i < count; i++) {
        jobject player = env->GetObjectArrayElement(players, i);
        if (env->IsSameObject(player, localPlayer)) { env->DeleteLocalRef(player); continue; }
        if (env->GetBooleanField(player, isDeadField)) { env->DeleteLocalRef(player); continue; }

        double pX = env->GetDoubleField(player, posXField);
        double pZ = env->GetDoubleField(player, posZField);
        double dist = sqrt(pow(pX - lpX, 2) + pow(pZ - lpZ, 2));

        if (dist < bestDist) {
            bestDist = dist;
            if (bestTarget) env->DeleteLocalRef(bestTarget);
            bestTarget = player;
        } else {
            env->DeleteLocalRef(player);
        }
    }

    if (bestTarget) {
        double pX = env->GetDoubleField(bestTarget, posXField);
        double pZ = env->GetDoubleField(bestTarget, posZField);
        float pYaw = env->GetFloatField(bestTarget, rotationYawField);

        float moveForward = 0;
        float moveStrafe = 0;

        // 1. Vertical Logic (Stay at vertex distance)
        float targetDist = getSetting("Target Distance")->value;
        
        // Facing Awareness: If enemy is looking at us, back up slightly to stay out of reach
        float angleToUs = (float)(atan2(lpZ - pZ, lpX - pX) * 180.0 / M_PI) - 90.0f;
        float facingDiff = pYaw - angleToUs;
        while (facingDiff < -180) facingDiff += 360;
        while (facingDiff > 180) facingDiff -= 360;

        bool enemyLookingAtUs = abs(facingDiff) < 40.0f;
        float dynamicTargetDist = enemyLookingAtUs ? targetDist + 0.2f : targetDist - 0.1f;

        if (bestDist > dynamicTargetDist + 0.1) moveForward = 1.0f;
        else if (bestDist < dynamicTargetDist - 0.1) moveForward = -1.0f;

        // 2. Circular Logic (Strafe)
        if (now - lastSwitchTime > 1200) {
            lastStrafeDir *= -1.0f;
            lastSwitchTime = now;
        }
        
        // If enemy is looking at us, strafe faster/harder to "break" their aim
        moveStrafe = enemyLookingAtUs ? lastStrafeDir * 1.2f : lastStrafeDir;

        // 3. Hole Awareness
        if (getSetting("Avoid Holes")->getBool()) {
            double radYaw = (lpYaw) * M_PI / 180.0;
            // Simplified projection check
            double lookX = -sin(radYaw);
            double lookZ = cos(radYaw);
            double sideX = cos(radYaw);
            double sideZ = sin(radYaw);

            double projX = lpX + (lookX * moveForward + sideX * moveStrafe) * 1.2;
            double projZ = lpZ + (lookZ * moveForward + sideZ * moveStrafe) * 1.2;

            if (blockPosClass && blockPosConstructor && isAirBlockMethod) {
                jobject posBelow = env->NewObject(blockPosClass, blockPosConstructor, (int)floor(projX), (int)floor(lpY - 0.5), (int)floor(projZ));
                if (env->CallBooleanMethod(world, isAirBlockMethod, posBelow)) {
                    // Hole detected! Reverse strafe or stop
                    moveStrafe *= -1.0f; 
                    moveForward = 0.0f;
                    lastStrafeDir = moveStrafe;
                    lastSwitchTime = now;
                }
                env->DeleteLocalRef(posBelow);
            }
        }

        // 4. Apply to MovementInput (Nudge)
        jobject mInput = env->GetObjectField(localPlayer, movementInputInstanceField);
        if (mInput) {
            float strength = getSetting("Strength")->value;
            // Only modify if user is already trying to move (optional, but makes it ghostlier)
            // For now, full assist as requested.
            env->SetFloatField(mInput, moveForwardField, moveForward * strength);
            env->SetFloatField(mInput, moveStrafeField, moveStrafe * strength);
            env->DeleteLocalRef(mInput);
        }

        env->DeleteLocalRef(bestTarget);
    }

    env->DeleteLocalRef(players);
    env->DeleteLocalRef(playersList);
    env->DeleteLocalRef(world);
    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
}
