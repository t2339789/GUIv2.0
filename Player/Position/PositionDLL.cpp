#include <windows.h>
#include <jni.h>
#include <jvmti.h>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

typedef jint (JNICALL *GetCreatedJavaVMs)(JavaVM**, jsize, jsize*);

struct PlayerData {
    std::string name;
    double x, y, z;
    double distance;
};

void Log(const std::string& msg) {
    std::ofstream logFile("C:\\Users\\JaeB8\\Player\\Position\\position.txt", std::ios::app);
    logFile << msg << std::endl;
}

jclass FindClassJVMTI(jvmtiEnv* jvmti, JNIEnv* env, const std::string& className) {
    jclass* classes = nullptr;
    jint count = 0;
    if (jvmti->GetLoadedClasses(&count, &classes) != JVMTI_ERROR_NONE) return nullptr;
    std::string sig = "L" + className + ";";
    jclass found = nullptr;
    for (int i = 0; i < count; i++) {
        char* signature = nullptr;
        if (jvmti->GetClassSignature(classes[i], &signature, nullptr) == JVMTI_ERROR_NONE) {
            if (signature && sig == signature) {
                found = classes[i];
                jvmti->Deallocate((unsigned char*)signature);
                break;
            }
            if (signature) jvmti->Deallocate((unsigned char*)signature);
        }
    }
    jvmti->Deallocate((unsigned char*)classes);
    return found;
}

void Run() {
    Log("--- Nearby Players Scan (v11 - Success Implementation) ---");
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

    {
        jclass mcClass = FindClassJVMTI(jvmti, env, "ave");
        if (!mcClass) { Log("Fail: ave"); goto cleanup; }

        jfieldID mcInstanceField = env->GetStaticFieldID(mcClass, "S", "Lave;");
        jobject mcInstance = env->GetStaticObjectField(mcClass, mcInstanceField);
        if (!mcInstance) { Log("Fail: mcInstance"); goto cleanup; }

        jfieldID localPlayerField = env->GetFieldID(mcClass, "h", "Lbew;");
        jobject localPlayer = env->GetObjectField(mcInstance, localPlayerField);

        jfieldID worldField = env->GetFieldID(mcClass, "f", "Lbdb;");
        jobject world = env->GetObjectField(mcInstance, worldField);
        if (!world) { Log("Fail: world"); goto cleanup; }

        jclass worldClass = env->GetObjectClass(world);
        jfieldID playersListField = env->GetFieldID(worldClass, "j", "Ljava/util/List;");
        jobject playersList = env->GetObjectField(world, playersListField);
        if (!playersList) { Log("Fail: playersList"); goto cleanup; }

        jclass listClass = env->FindClass("java/util/List");
        jmethodID toArrayMethod = env->GetMethodID(listClass, "toArray", "()[Ljava/lang/Object;");
        jobjectArray playersArray = (jobjectArray)env->CallObjectMethod(playersList, toArrayMethod);
        jint count = env->GetArrayLength(playersArray);

        jclass entityClass = FindClassJVMTI(jvmti, env, "pk");
        jfieldID xField = env->GetFieldID(entityClass, "s", "D");
        jfieldID yField = env->GetFieldID(entityClass, "t", "D");
        jfieldID zField = env->GetFieldID(entityClass, "u", "D");

        jclass playerClass = FindClassJVMTI(jvmti, env, "wn");
        jmethodID nameMethod = env->GetMethodID(playerClass, "e_", "()Ljava/lang/String;");

        double lx = 0, ly = 0, lz = 0;
        if (localPlayer) {
            lx = env->GetDoubleField(localPlayer, xField);
            ly = env->GetDoubleField(localPlayer, yField);
            lz = env->GetDoubleField(localPlayer, zField);
            Log("Local Position: X=" + std::to_string(lx) + ", Y=" + std::to_string(ly) + ", Z=" + std::to_string(lz));
        }

        std::vector<PlayerData> players;
        for (int i = 0; i < count; i++) {
            jobject p = env->GetObjectArrayElement(playersArray, i);
            if (!p || (localPlayer && env->IsSameObject(p, localPlayer))) continue;
            if (!env->IsInstanceOf(p, playerClass)) continue;

            double px = env->GetDoubleField(p, xField);
            double py = env->GetDoubleField(p, yField);
            double pz = env->GetDoubleField(p, zField);
            double dist = std::sqrt(std::pow(px - lx, 2) + std::pow(py - ly, 2) + std::pow(pz - lz, 2));

            std::string name = "Unknown";
            if (nameMethod) {
                jstring jname = (jstring)env->CallObjectMethod(p, nameMethod);
                if (jname) {
                    const char* nameStr = env->GetStringUTFChars(jname, nullptr);
                    if (nameStr) {
                        name = nameStr;
                        env->ReleaseStringUTFChars(jname, nameStr);
                    }
                }
                env->ExceptionClear();
            }
            players.push_back({name, px, py, pz, dist});
        }

        std::sort(players.begin(), players.end(), [](const PlayerData& a, const PlayerData& b) {
            return a.distance < b.distance;
        });

        Log("Found " + std::to_string(players.size()) + " other players:");
        for (int i = 0; i < (int)players.size(); i++) {
            const auto& p = players[i];
            Log("#" + std::to_string(i+1) + ": " + p.name + " | Dist: " + std::to_string(p.distance) + 
                " | X: " + std::to_string(p.x) + " Y: " + std::to_string(p.y) + " Z: " + std::to_string(p.z));
        }
    }

cleanup:
    Log("Done.");
    vm->DetachCurrentThread();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Run, NULL, 0, NULL);
    }
    return TRUE;
}
