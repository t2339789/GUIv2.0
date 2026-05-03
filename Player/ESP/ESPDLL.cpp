#include <windows.h>
#include <GL/gl.h>
#include <gdiplus.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <jni.h>
#include <jvmti.h>
#include <fstream>
#include <string>
#include "MinHook.h"
#include "../GUI/ModuleManager.h"
#include "../GUI/ClickGUI.h"
#include "../GUI/PathUtils.h"

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

typedef jint (JNICALL *GetCreatedJavaVMs)(JavaVM**, jsize, jsize*);
JavaVM* g_vm = nullptr;
jvmtiEnv* g_jvmti = nullptr;
ULONG_PTR g_gdiToken = 0;

void DebugLog(const std::string& msg) {
    std::ofstream f(PathUtils::GetPathA("ESP\\debug.txt"), std::ios::app);
    f << msg << std::endl;
}

jfieldID GetFieldReflective(JNIEnv* env, jclass clazz, const char* name, const char* sig) {
    jfieldID fid = env->GetFieldID(clazz, name, sig);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return fid;
}

jclass FindClassJVMTI(JNIEnv* env, const char* name) {
    if (!g_jvmti || !name) return nullptr;
    std::string className(name);
    jclass* classes = nullptr;
    jint count = 0;
    if (g_jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE) return nullptr;
    std::string sig = "L" + className + ";";
    jclass found = nullptr;
    for (int i = 0; i < count; i++) {
        char* s = nullptr;
        if (g_jvmti->GetClassSignature(classes[i], &s, nullptr) == JVMTI_ERROR_NONE) {
            if (s && sig == s) { found = classes[i]; g_jvmti->Deallocate((unsigned char*)s); break; }
            if (s) g_jvmti->Deallocate((unsigned char*)s);
        }
    }
    g_jvmti->Deallocate((unsigned char*)classes);
    return found;
}

bool g_initialized = false;
bool g_gdiInitialized = false;
jclass mcClass, timerClass, entityClass, worldClass, listClass, playerClass, fontRendererClass;
jfieldID mcInstanceField, localPlayerField, worldField, renderManagerField, timerField, fontRendererField;
jfieldID viewerPosXField, viewerPosYField, viewerPosZField, playerViewYField, playerViewXField;
jfieldID partialTicksField, lastTickPosXField, lastTickPosYField, lastTickPosZField, posXField, posYField, posZField;
jfieldID motionXField, motionYField, motionZField;
jfieldID playersListField, entityListField, heightField, widthField, isDeadField, rotationPitchField, rotationYawField;
jfieldID leftClickCounterField, hurtTimeField, objectMouseOverField, entityHitField;
jmethodID toArrayMethod, drawStringMethod, isSameTeamMethod, isInvisibleMethod, getTeamMethod, jumpMethod;
jfieldID locationFontTextureField, renderEngineField;
jmethodID bindTextureMethod;
jfieldID sendQueueField, onGroundField, movementInputInstanceField, moveForwardField, moveStrafeField;
jmethodID sendPacketMethod, isAirBlockMethod, blockPosConstructor, lookPacketConstructor, getHealthMethod, getMaxHealthMethod, sendChatMessageMethod;
jclass lookPacketClass, entityLivingBaseClass, movementInputClass, blockPosClass, customPayloadClass, byteBufClass, packetBufferClass;
jmethodID customPayloadConstructor, writeByteMethod, writeIntMethod, writeStringMethod, packetBufferConstructor;

void InitJniGlobals(JNIEnv* env) {
    if (g_initialized) return;

    if (!g_gdiInitialized) {
        GdiplusStartupInput gdiInput;
        GdiplusStartup(&g_gdiToken, &gdiInput, NULL);
        g_gdiInitialized = true;
    }

    jclass localMcClass = FindClassJVMTI(env, "ave");
    if (!localMcClass) return;
    mcClass = (jclass)env->NewGlobalRef(localMcClass);
    
    mcInstanceField = env->GetStaticFieldID(mcClass, "S", "Lave;");
    localPlayerField = env->GetFieldID(mcClass, "h", "Lbew;");
    worldField = env->GetFieldID(mcClass, "f", "Lbdb;");
    renderManagerField = env->GetFieldID(mcClass, "aa", "Lbiu;");
    timerField = env->GetFieldID(mcClass, "Y", "Lavl;");
    objectMouseOverField = env->GetFieldID(mcClass, "s", "Laui;");
    if (!objectMouseOverField) { env->ExceptionClear(); objectMouseOverField = env->GetFieldID(mcClass, "objectMouseOver", "Lnet/minecraft/util/MovingObjectPosition;"); }
    leftClickCounterField = env->GetFieldID(mcClass, "ag", "I");
    
    jclass localBiuClass = FindClassJVMTI(env, "biu");
    if (localBiuClass) {
        viewerPosXField = env->GetFieldID(localBiuClass, "h", "D");
        viewerPosYField = env->GetFieldID(localBiuClass, "i", "D");
        viewerPosZField = env->GetFieldID(localBiuClass, "j", "D");
        playerViewYField = env->GetFieldID(localBiuClass, "e", "F");
        playerViewXField = env->GetFieldID(localBiuClass, "f", "F");
    }

    jclass localTimerClass = FindClassJVMTI(env, "avl");
    if (localTimerClass) {
        timerClass = (jclass)env->NewGlobalRef(localTimerClass);
        partialTicksField = env->GetFieldID(timerClass, "c", "F");
    }

    jclass localEntityClass = FindClassJVMTI(env, "pk");
    if (localEntityClass) {
        entityClass = (jclass)env->NewGlobalRef(localEntityClass);
        lastTickPosXField = env->GetFieldID(entityClass, "p", "D");
        lastTickPosYField = env->GetFieldID(entityClass, "q", "D");
        lastTickPosZField = env->GetFieldID(entityClass, "r", "D");
        posXField = env->GetFieldID(entityClass, "s", "D");
        posYField = env->GetFieldID(entityClass, "t", "D");
        posZField = env->GetFieldID(entityClass, "u", "D");
        motionXField = env->GetFieldID(entityClass, "v", "D");
        motionYField = env->GetFieldID(entityClass, "w", "D");
        motionZField = env->GetFieldID(entityClass, "x", "D");
        rotationYawField = env->GetFieldID(entityClass, "y", "F");
        rotationPitchField = env->GetFieldID(entityClass, "z", "F");
        widthField = env->GetFieldID(entityClass, "J", "F");
        heightField = env->GetFieldID(entityClass, "K", "F");
        isInvisibleMethod = env->GetMethodID(entityClass, "ax", "()Z");
    }

    jclass localLivingClass = FindClassJVMTI(env, "pr");
    if (localLivingClass) {
        entityLivingBaseClass = (jclass)env->NewGlobalRef(localLivingClass);
        getHealthMethod = env->GetMethodID(entityLivingBaseClass, "bn", "()F");
        getMaxHealthMethod = env->GetMethodID(entityLivingBaseClass, "bu", "()F");
        hurtTimeField = env->GetFieldID(entityLivingBaseClass, "au", "I");
        isDeadField = env->GetFieldID(entityLivingBaseClass, "aI", "Z");
        jumpMethod = env->GetMethodID(entityLivingBaseClass, "bZ", "()V");
    }

    jclass localWorldClass = FindClassJVMTI(env, "bdb");
    if (localWorldClass) {
        worldClass = (jclass)env->NewGlobalRef(localWorldClass);
        playersListField = env->GetFieldID(worldClass, "j", "Ljava/util/List;");
        entityListField = env->GetFieldID(worldClass, "f", "Ljava/util/List;");
    }

    jclass localListClass = env->FindClass("java/util/List");
    if (localListClass) {
        listClass = (jclass)env->NewGlobalRef(localListClass);
        toArrayMethod = env->GetMethodID(listClass, "toArray", "()[Ljava/lang/Object;");
    }

    jclass localPlayerClass = FindClassJVMTI(env, "wn");
    if (localPlayerClass) {
        playerClass = (jclass)env->NewGlobalRef(localPlayerClass);
        getTeamMethod = env->GetMethodID(playerClass, "bO", "()Lauq;");
        isSameTeamMethod = env->GetMethodID(playerClass, "c", "(Lpk;)Z");
    }

    jclass localMInputClass = FindClassJVMTI(env, "avp");
    if (localMInputClass) {
        movementInputClass = (jclass)env->NewGlobalRef(localMInputClass);
        moveForwardField = env->GetFieldID(movementInputClass, "g", "F");
        moveStrafeField = env->GetFieldID(movementInputClass, "h", "F");
    }

    jclass localPlayerSPClass = FindClassJVMTI(env, "bew");
    if (localPlayerSPClass) {
        movementInputInstanceField = env->GetFieldID(localPlayerSPClass, "b", "Lavp;");
        sendChatMessageMethod = env->GetMethodID(localPlayerSPClass, "e", "(Ljava/lang/String;)V");
    }

    jclass localBlockPosClass = FindClassJVMTI(env, "cj");
    if (localBlockPosClass) {
        blockPosClass = (jclass)env->NewGlobalRef(localBlockPosClass);
        blockPosConstructor = env->GetMethodID(blockPosClass, "<init>", "(III)V");
    }

    if (worldClass) {
        isAirBlockMethod = env->GetMethodID(worldClass, "d", "(Lcj;)Z");
    }

    fontRendererField = env->GetFieldID(mcClass, "k", "Lavn;");
    if (!fontRendererField) { env->ExceptionClear(); fontRendererField = env->GetFieldID(mcClass, "fontRendererObj", "Lnet/minecraft/client/gui/FontRenderer;"); }
    
    jclass localFontClass = FindClassJVMTI(env, "avn");
    if (localFontClass) {
        fontRendererClass = (jclass)env->NewGlobalRef(localFontClass);
        drawStringMethod = env->GetMethodID(fontRendererClass, "a", "(Ljava/lang/String;III)I");
        locationFontTextureField = env->GetFieldID(fontRendererClass, "a", "Ljy;");
        renderEngineField = env->GetFieldID(fontRendererClass, "c", "Lbmg;");
        if (!renderEngineField) {
            env->ExceptionClear();
            renderEngineField = env->GetFieldID(fontRendererClass, "renderEngine", "Lnet/minecraft/client/renderer/texture/TextureManager;");
            locationFontTextureField = env->GetFieldID(fontRendererClass, "locationFontTexture", "Lnet/minecraft/util/ResourceLocation;");
        }
    }
    
    jclass tmClass = FindClassJVMTI(env, "bmg");
    if (tmClass) {
        bindTextureMethod = env->GetMethodID(tmClass, "a", "(Ljy;)V");
        if (!bindTextureMethod) {
            env->ExceptionClear();
            bindTextureMethod = env->GetMethodID(tmClass, "bindTexture", "(Lnet/minecraft/util/ResourceLocation;)V");
        }
    }
    
    jclass localLookPacketClass = FindClassJVMTI(env, "iw");
    if (localLookPacketClass) {
        lookPacketClass = (jclass)env->NewGlobalRef(localLookPacketClass);
        lookPacketConstructor = env->GetMethodID(lookPacketClass, "<init>", "(FFZ)V");
    }

    sendQueueField = env->GetFieldID(FindClassJVMTI(env, "bew"), "a", "Lbcy;");
    jclass localNetHandlerClass = FindClassJVMTI(env, "bcy");
    if (localNetHandlerClass) {
        sendPacketMethod = env->GetMethodID(localNetHandlerClass, "a", "(Lff;)V");
    }

    jclass localCPClass = FindClassJVMTI(env, "fp");
    if (localCPClass) {
        customPayloadClass = (jclass)env->NewGlobalRef(localCPClass);
        customPayloadConstructor = env->GetMethodID(customPayloadClass, "<init>", "(Ljava/lang/String;Lem;)V");
    }

    jclass localPBClass = FindClassJVMTI(env, "em");
    if (localPBClass) {
        packetBufferClass = (jclass)env->NewGlobalRef(localPBClass);
        packetBufferConstructor = env->GetMethodID(packetBufferClass, "<init>", "(Lio/netty/buffer/ByteBuf;)V");
        writeStringMethod = env->GetMethodID(packetBufferClass, "a", "(Ljava/lang/String;)Lem;");
    }

    jclass localBBClass = env->FindClass("io/netty/buffer/ByteBuf");
    if (localBBClass) {
        byteBufClass = (jclass)env->NewGlobalRef(localBBClass);
        writeByteMethod = env->GetMethodID(byteBufClass, "writeByte", "(I)Lio/netty/buffer/ByteBuf;");
        writeIntMethod = env->GetMethodID(byteBufClass, "writeInt", "(I)Lio/netty/buffer/ByteBuf;");
    }

    onGroundField = env->GetFieldID(entityClass, "C", "Z");

    jclass mopClass = FindClassJVMTI(env, "aui");
    if (mopClass) {
        entityHitField = env->GetFieldID(mopClass, "d", "Lpk;");
    }

    env->ExceptionClear();
    ModuleManager::init();
    ClickGUI::init();
    g_initialized = true;
}

typedef BOOL (WINAPI *twglSwapBuffers)(HDC hDc);
twglSwapBuffers owglSwapBuffers = nullptr;

BOOL WINAPI hwglSwapBuffers(HDC hDc) {
    if (!g_vm) return owglSwapBuffers(hDc);

    JNIEnv* env = nullptr;
    if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK || !env) {
        return owglSwapBuffers(hDc);
    }

    if (!g_initialized) {
        InitJniGlobals(env);
    }

    if (g_initialized) {
        ModuleManager::render3D();
        ClickGUI::render(hDc, env);
    }

    return owglSwapBuffers(hDc);
}

void RunHook() {
    HMODULE hJvm = GetModuleHandleA("jvm.dll");
    if (hJvm) {
        auto pGetCreatedJavaVMs = (GetCreatedJavaVMs)GetProcAddress(hJvm, "JNI_GetCreatedJavaVMs");
        jsize nVMs; pGetCreatedJavaVMs(&g_vm, 1, &nVMs);
        if (g_vm) {
            JNIEnv* env = nullptr;
            g_vm->AttachCurrentThread((void**)&env, nullptr);
            g_vm->GetEnv((void**)&g_jvmti, JVMTI_VERSION_1_2);
            g_vm->DetachCurrentThread();
        }
    }
    if (MH_Initialize() != MH_OK) return;
    HMODULE hOpengl32 = GetModuleHandleA("opengl32.dll");
    if (!hOpengl32) hOpengl32 = LoadLibraryA("opengl32.dll");
    if (hOpengl32) {
        void* pWglSwapBuffers = (void*)GetProcAddress(hOpengl32, "wglSwapBuffers");
        if (pWglSwapBuffers && MH_CreateHook(pWglSwapBuffers, (void*)&hwglSwapBuffers, (void**)&owglSwapBuffers) == MH_OK) {
            MH_EnableHook(pWglSwapBuffers);
        }
    }
}

void Unhook(JNIEnv* env) {
    if (owglSwapBuffers) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
    
    ModuleManager::cleanup();
    
    if (env) {
        if (mcClass) env->DeleteGlobalRef(mcClass);
        if (timerClass) env->DeleteGlobalRef(timerClass);
        if (entityClass) env->DeleteGlobalRef(entityClass);
        if (entityLivingBaseClass) env->DeleteGlobalRef(entityLivingBaseClass);
        if (worldClass) env->DeleteGlobalRef(worldClass);
        if (listClass) env->DeleteGlobalRef(listClass);
        if (playerClass) env->DeleteGlobalRef(playerClass);
        if (fontRendererClass) env->DeleteGlobalRef(fontRendererClass);
        if (lookPacketClass) env->DeleteGlobalRef(lookPacketClass);
    }

    if (g_gdiToken) {
        GdiplusShutdown(g_gdiToken);
        g_gdiToken = 0;
    }
}

void EjectThread(HMODULE hModule) {
    while (!(GetAsyncKeyState(VK_END) & 1)) Sleep(100);
    
    JNIEnv* env = nullptr;
    if (g_vm) {
        g_vm->AttachCurrentThread((void**)&env, nullptr);
    }
    
    Unhook(env);
    
    if (g_vm) {
        g_vm->DetachCurrentThread();
    }
    
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RunHook, NULL, 0, NULL);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EjectThread, hModule, 0, NULL);
    }
    return TRUE;
}
