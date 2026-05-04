#include "RotationSyncModule.h"
#include <windows.h>
#include <jni.h>

extern JavaVM* g_vm;
extern bool g_initialized;
extern jclass mcClass, lookPacketClass;
extern jfieldID mcInstanceField, localPlayerField, rotationYawField, rotationPitchField, onGroundField, sendQueueField;
extern jmethodID lookPacketConstructor, sendPacketMethod;

RotationSyncModule::RotationSyncModule() : Module("RotationSync", true), wasLMBPressed(false) {}

void RotationSyncModule::onRender2D() {
    if (!enabled || !g_vm || !g_initialized) return;

    bool isLMB = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool justClicked = isLMB && !wasLMBPressed;
    wasLMBPressed = isLMB;

    if (!justClicked) return;

    JNIEnv* env = nullptr;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK || !env) return;

    jobject mc = env->GetStaticObjectField(mcClass, mcInstanceField);
    if (!mc) return;

    jobject localPlayer = env->GetObjectField(mc, localPlayerField);
    if (!localPlayer) { env->DeleteLocalRef(mc); return; }

    // Manually push a rotation packet to the server immediately before the game's native attack packet.
    // This ensures that even with low TPS or high latency, the server sees you looking at the target
    // at the moment of the hit.
    if (sendQueueField && sendPacketMethod && lookPacketClass && lookPacketConstructor) {
        jobject sendQueue = env->GetObjectField(localPlayer, sendQueueField);
        if (sendQueue) {
            float yaw = env->GetFloatField(localPlayer, rotationYawField);
            float pitch = env->GetFloatField(localPlayer, rotationPitchField);
            jboolean onGround = env->GetBooleanField(localPlayer, onGroundField);
            
            jobject lookPacket = env->NewObject(lookPacketClass, lookPacketConstructor, (jfloat)yaw, (jfloat)pitch, onGround);
            if (lookPacket) {
                env->CallVoidMethod(sendQueue, sendPacketMethod, lookPacket);
                env->DeleteLocalRef(lookPacket);
            }
            env->DeleteLocalRef(sendQueue);
        }
    }

    env->DeleteLocalRef(localPlayer);
    env->DeleteLocalRef(mc);
}
