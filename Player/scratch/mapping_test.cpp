#include <windows.h>
#include <jni.h>
#include <jvmti.h>
#include <string>
#include <vector>

typedef jint (JNICALL *GetCreatedJavaVMs)(JavaVM**, jsize, jsize*);

void Log(const std::string& msg) {
    FILE* f = fopen("c:\\Users\\JaeB8\\Player\\scratch\\mapping_test.txt", "a");
    if (f) {
        fprintf(f, "%s\n", msg.c_str());
        fclose(f);
    }
}

jclass FindClassJVMTI(jvmtiEnv* jvmti, const std::string& className) {
    jclass* classes = nullptr;
    jint count = 0;
    if (jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE) return nullptr;
    std::string sig = "L" + className + ";";
    jclass found = nullptr;
    for (int i = 0; i < count; i++) {
        char* s = nullptr;
        if (jvmti->GetClassSignature(classes[i], &s, nullptr) == JVMTI_ERROR_NONE) {
            if (s && sig == s) { found = classes[i]; jvmti->Deallocate((unsigned char*)s); break; }
            if (s) jvmti->Deallocate((unsigned char*)s);
        }
    }
    jvmti->Deallocate((unsigned char*)classes);
    return found;
}

void Run() {
    Log("--- Mapping Test ---");
    HMODULE hJvm = GetModuleHandleA("jvm.dll");
    if (!hJvm) return;
    auto pGetCreatedJavaVMs = (GetCreatedJavaVMs)GetProcAddress(hJvm, "JNI_GetCreatedJavaVMs");
    JavaVM* vm = nullptr;
    jsize nVMs;
    if (pGetCreatedJavaVMs(&vm, 1, &nVMs) != JNI_OK || nVMs == 0) return;
    JNIEnv* env = nullptr;
    if (vm->AttachCurrentThread((void**)&env, nullptr) != JNI_OK) return;
    jvmtiEnv* jvmti = nullptr;
    vm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_2);

    const char* classes[] = {"ja", "iu", "iv", "iw", "ix", "ek", "bcy", "ff"};
    for (const char* c : classes) {
        jclass cls = FindClassJVMTI(jvmti, c);
        if (cls) {
            Log("Found class: " + std::string(c));
        } else {
            Log("Failed to find class: " + std::string(c));
        }
    }

    vm->DetachCurrentThread();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Run, NULL, 0, NULL);
    }
    return TRUE;
}
