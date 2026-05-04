#pragma once
#include "../Module.h"
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <thread>
#include <jni.h>

struct PlayerStats {
    int level = 0;
    int bwStars = 0;
    int finalKills = 0;
    int finalDeaths = 0;
    float fkdr = 0.0f;
    bool loaded = false;
    bool loading = false;
    unsigned long long lastUpdate = 0;
};

class StatCheckerModule : public Module {
private:
    std::string apiKey = "f206e14d-7966-450c-b18c-c68fe0bcdce8";
    std::map<std::string, PlayerStats> cache;
    std::mutex cacheMutex;
    void fetchStats(void* hS, std::string name);
    void fetchStatsPlancke(void* hS, std::string name);
    std::string getName(JNIEnv* env, jobject player);
    
    void saveCache();
    void loadCache();

    // Background Worker
    std::queue<std::string> requestQueue;
    std::condition_variable queueCV;
    std::thread workerThread;
    bool stopWorker = false;
    void workerLoop();
    
    // Persistent Network Handles
    void* hSession = nullptr;
    void* hConnectHypixel = nullptr;
    void* hConnectPlancke = nullptr;

public:
    StatCheckerModule();
    ~StatCheckerModule();
    virtual void onRender2D() override;
    
    // New centralized rendering called directly from ClickGUI
    void drawStats(JNIEnv* env, jobject fontRenderer, int sw, int sh);
};
