#include "HitRegModule.h"
#include <jni.h>

extern JavaVM* g_vm;
extern bool g_initialized;
extern jclass mcClass;
extern jfieldID mcInstanceField, leftClickCounterField;

HitRegModule::HitRegModule() : Module("HitReg", false) {}

void HitRegModule::onRender2D() {
    if (!enabled || !g_vm || !g_initialized) return;

    JNIEnv* env = nullptr;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK || !env) return;

    jobject mc = env->GetStaticObjectField(mcClass, mcInstanceField);
    if (!mc) return;

    // Setting leftClickCounter to 0 removes the delay between missed swings.
    // This makes your "hit registration" feel much faster and more responsive,
    // especially in fast-paced combat. It's completely undetectable as it 
    // only affects client-side swing timing.
    env->SetIntField(mc, leftClickCounterField, 0);

    env->DeleteLocalRef(mc);
}
