#include "VelocityModule.h"
#include <windows.h>
#include <jni.h>
#include <cmath>
#include <algorithm>
#include <iostream>

extern JavaVM* g_vm;
extern bool g_initialized;
extern jclass mcClass, worldClass, playerClass;
extern jfieldID mcInstanceField, localPlayerField, rotationYawField, hurtTimeField, worldField, playersListField, posXField, posZField;
extern jfieldID motionXField, motionYField, motionZField, onGroundField;
extern jmethodID toArrayMethod;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern jmethodID getTeamMethod, isSameTeamMethod, jumpMethod;

VelocityModule::VelocityModule() : Module("Velocity", false), targetYaw(0), needsRotation(false) {
    addSetting("Horizontal", 100.0f, 0.0f, 100.0f);
    addSetting("Vertical", 100.0f, 0.0f, 100.0f);
    addSetting("Jump Reset", true);
    addSetting("Jump Chance", 80.0f, 0.0f, 100.0f);
    addSetting("Friction", 0.95f, 0.8f, 1.0f);
    addSetting("W-Tap", true);
}

void VelocityModule::onRender2D() {
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

    int hurtTime = env->GetIntField(localPlayer, hurtTimeField);
    float currentYaw = env->GetFloatField(localPlayer, rotationYawField);
    jboolean onGround = env->GetBooleanField(localPlayer, onGroundField);

    static int lastHurtTime = 0;
    static unsigned long long releaseTime = 0;
    static BYTE activeKey = 0;

    // --- Physics-Based Velocity ---
    if (hurtTime == 10 && lastHurtTime < 10) {

        // 1. Jump Reset
        if (getSetting("Jump Reset")->getBool() && onGround) {
            float chance = getSetting("Jump Chance")->value;

            if ((rand() % 100) < (int)chance) {
                if (jumpMethod) {
                    env->CallVoidMethod(localPlayer, jumpMethod);
                }
            }
        }

        // 2. Initial Hit Reduction
        float horizontal = getSetting("Horizontal")->value / 100.0f;
        float vertical = getSetting("Vertical")->value / 100.0f;

        if (onGround && (horizontal < 1.0f || vertical < 1.0f)) {
            double mx = env->GetDoubleField(localPlayer, motionXField);
            double my = env->GetDoubleField(localPlayer, motionYField);
            double mz = env->GetDoubleField(localPlayer, motionZField);

            env->SetDoubleField(localPlayer, motionXField, mx * horizontal);
            env->SetDoubleField(localPlayer, motionYField, my * vertical);
            env->SetDoubleField(localPlayer, motionZField, mz * horizontal);
        }

        // 3. W-Tap / Sprint Reset
        if (getSetting("W-Tap")->getBool()) {
            jobject world = env->GetObjectField(mc, worldField);

            if (world) {
                jobject lpTeam = nullptr;

                if (getTeamMethod) {
                    lpTeam = env->CallObjectMethod(localPlayer, getTeamMethod);
                    if (env->ExceptionCheck()) {
                        env->ExceptionClear();
                        lpTeam = nullptr;
                    }
                }

                double lpX = env->GetDoubleField(localPlayer, posXField);
                double lpZ = env->GetDoubleField(localPlayer, posZField);

                jobject playersList = env->GetObjectField(world, playersListField);

                if (playersList) {
                    jobjectArray players = (jobjectArray)env->CallObjectMethod(playersList, toArrayMethod);

                    if (players) {
                        int count = env->GetArrayLength(players);

                        double minDist = 4.5;
                        jobject nearestEnemy = nullptr;

                        for (int i = 0; i < count; i++) {
                            jobject player = env->GetObjectArrayElement(players, i);
                            if (!player) continue;

                            if (env->IsSameObject(player, localPlayer)) {
                                env->DeleteLocalRef(player);
                                continue;
                            }

                            if (lpTeam && getTeamMethod && isSameTeamMethod) {
                                jobject team = env->CallObjectMethod(player, getTeamMethod);

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
                                        env->DeleteLocalRef(player);
                                        continue;
                                    }
                                }
                            }

                            double pX = env->GetDoubleField(player, posXField);
                            double pZ = env->GetDoubleField(player, posZField);
                            double dist = sqrt(pow(pX - lpX, 2) + pow(pZ - lpZ, 2));

                            if (dist < minDist) {
                                minDist = dist;

                                if (nearestEnemy) {
                                    env->DeleteLocalRef(nearestEnemy);
                                }

                                nearestEnemy = player;
                            } else {
                                env->DeleteLocalRef(player);
                            }
                        }

                        if (nearestEnemy) {
                            double pX = env->GetDoubleField(nearestEnemy, posXField);
                            double pZ = env->GetDoubleField(nearestEnemy, posZField);

                            float angleToEnemy = (float)(atan2(pZ - lpZ, pX - lpX) * 180.0 / M_PI) - 90.0f;

                            float relYaw = angleToEnemy - currentYaw;
                            while (relYaw <= -180.0f) relYaw += 360.0f;
                            while (relYaw > 180.0f) relYaw -= 360.0f;

                            // Release previous synthetic key if there is one.
                            if (activeKey != 0) {
                                keybd_event(activeKey, 0, KEYEVENTF_KEYUP, 0);
                                activeKey = 0;
                            }

                            BYTE newKey = 0;

                            if (relYaw >= -45.0f && relYaw <= 45.0f) {
                                newKey = 'W';
                            } else if (relYaw > 45.0f && relYaw <= 135.0f) {
                                newKey = 'D';
                            } else if (relYaw < -45.0f && relYaw >= -135.0f) {
                                newKey = 'A';
                            } else {
                                // Enemy is behind you. Do NOT press S.
                                newKey = 0;
                            }

                            if (newKey != 0) {
                                activeKey = newKey;

                                keybd_event(activeKey, 0, KEYEVENTF_KEYUP, 0);
                                keybd_event(activeKey, 0, 0, 0);

                                releaseTime = GetTickCount64() + 60;
                            } else {
                                releaseTime = 0;
                            }

                            env->DeleteLocalRef(nearestEnemy);
                        }

                        env->DeleteLocalRef(players);
                    }

                    env->DeleteLocalRef(playersList);
                }

                if (lpTeam) {
                    env->DeleteLocalRef(lpTeam);
                }

                env->DeleteLocalRef(world);
            }
        }
    }

    // 4. Friction Manipulation
    static int lastProcessedHurtTime = -1;

    if (onGround && hurtTime > 0 && hurtTime < 10 && hurtTime != lastProcessedHurtTime) {
        float friction = getSetting("Friction")->value;

        if (friction < 1.0f) {
            double mx = env->GetDoubleField(localPlayer, motionXField);
            double mz = env->GetDoubleField(localPlayer, motionZField);

            env->SetDoubleField(localPlayer, motionXField, mx * friction);
            env->SetDoubleField(localPlayer, motionZField, mz * friction);
        }

        lastProcessedHurtTime = hurtTime;
    }

    if (hurtTime == 0) {
        lastProcessedHurtTime = -1;
    }

    lastHurtTime = hurtTime;

    // Release synthetic W/A/D key.
    if (activeKey != 0 && GetTickCount64() >= releaseTime) {
        if (!(GetAsyncKeyState(activeKey) & 0x8000)) {
            keybd_event(activeKey, 0, KEYEVENTF_KEYUP, 0);
        }

        activeKey = 0;
        releaseTime = 0;
    }

    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
}