#include "ComboBreakerModule.h"
#include <windows.h>
#include <jni.h>
#include <vector>
#include <cmath>

extern JavaVM* g_vm;
extern bool g_initialized;
extern jclass mcClass;
extern jfieldID mcInstanceField, localPlayerField, hurtTimeField, objectMouseOverField, entityHitField, onGroundField;
extern jfieldID worldField, posXField, posZField, playersListField;
extern jmethodID jumpMethod, toArrayMethod;

ComboBreakerModule::ComboBreakerModule() : Module("ComboBreaker", false), 
    hitsReceived(0), hitsDealt(0), lastMyHurtTime(0), lastTargetHurtTime(0), 
    lastHitTime(0), strafeEndTime(0), activeStrafeKey(0) 
{
    addSetting("Threshold", 3.0f, 2.0f, 5.0f);
    addSetting("Strafe Duration", 120.0f, 50.0f, 300.0f);
}

void ComboBreakerModule::onRender2D() {
    if (!enabled || !g_vm || !g_initialized) return;

    JNIEnv* env = nullptr;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK || !env) return;

    jobject mc = env->GetStaticObjectField(mcClass, mcInstanceField);
    if (!mc) return;

    jobject localPlayer = env->GetObjectField(mc, localPlayerField);
    if (!localPlayer) { env->DeleteLocalRef(mc); return; }

    unsigned long long now = GetTickCount64();

    // Reset counters if no combat for 2 seconds
    if (now - lastHitTime > 2000) {
        hitsReceived = 0;
        hitsDealt = 0;
    }

    // --- Hit Tracking ---
    int myHurtTime = env->GetIntField(localPlayer, hurtTimeField);
    if (myHurtTime == 10 && lastMyHurtTime < 10) {
        // If the last hit was more than 500ms ago, it's not a combo. Reset.
        if (now - lastHitTime > 500) {
            hitsReceived = 0;
        }
        hitsReceived++;
        lastHitTime = now;
    }
    lastMyHurtTime = myHurtTime;

    // --- Robust "Dealt Hit" Detection (to prevent triggering on trades) ---
    // Scan nearby entities to see if WE landed a hit.
    jobject world = env->GetObjectField(mc, worldField);
    if (world) {
        double lpX = env->GetDoubleField(localPlayer, posXField);
        double lpZ = env->GetDoubleField(localPlayer, posZField);
        jobject playersList = env->GetObjectField(world, playersListField);
        jobjectArray players = (jobjectArray)env->CallObjectMethod(playersList, toArrayMethod);
        int count = env->GetArrayLength(players);

        bool landedHit = false;
        extern jclass entityLivingBaseClass;

        for (int i = 0; i < count; i++) {
            jobject player = env->GetObjectArrayElement(players, i);
            if (env->IsSameObject(player, localPlayer)) { env->DeleteLocalRef(player); continue; }
            
            if (entityLivingBaseClass && env->IsInstanceOf(player, entityLivingBaseClass)) {
                int targetHurtTime = env->GetIntField(player, hurtTimeField);
                if (targetHurtTime == 10) { // This entity was JUST hit
                    double pX = env->GetDoubleField(player, posXField);
                    double pZ = env->GetDoubleField(player, posZField);
                    double dist = sqrt(pow(pX - lpX, 2) + pow(pZ - lpZ, 2));
                    
                    // If they are within reach and just took damage, assume WE hit them.
                    if (dist < 4.5) {
                        landedHit = true;
                    }
                }
            }
            env->DeleteLocalRef(player);
        }

        if (landedHit) {
            hitsReceived = 0; // RESET because we are trading or comboing them.
            lastHitTime = now;
        }

        env->DeleteLocalRef(players);
        env->DeleteLocalRef(playersList);
        env->DeleteLocalRef(world);
    }

    // --- The Trigger ---
    float threshold = getSetting("Threshold")->value;
    if (hitsReceived >= (int)threshold && activeStrafeKey == 0) {
        // COMBO DETECTED!
        
        // 1. Jump Reset (if on ground)
        jboolean onGround = env->GetBooleanField(localPlayer, onGroundField);
        if (onGround && jumpMethod) {
            env->CallVoidMethod(localPlayer, jumpMethod);
        }

        // 2. Emergency Side-Step (Strafe)
        activeStrafeKey = (rand() % 2 == 0) ? 'A' : 'D';
        keybd_event(activeStrafeKey, 0, 0, 0);
        strafeEndTime = now + (unsigned long long)getSetting("Strafe Duration")->value;

        // Reset received counter so we don't spam it every frame of the combo
        hitsReceived = 0; 
    }

    // Handle key release
    if (activeStrafeKey != 0 && now >= strafeEndTime) {
        if (!(GetAsyncKeyState(activeStrafeKey) & 0x8000)) {
            keybd_event(activeStrafeKey, 0, KEYEVENTF_KEYUP, 0);
        }
        activeStrafeKey = 0;
        strafeEndTime = 0;
    }

    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
}
