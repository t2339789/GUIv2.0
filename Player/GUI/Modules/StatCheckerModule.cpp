#include "StatCheckerModule.h"
#include "../ClickGUI.h"
#include "../PathUtils.h"
#include <windows.h>
#include <winhttp.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <fstream>

#pragma comment(lib, "winhttp.lib")

#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif

extern JavaVM* g_vm;
extern bool g_initialized;
extern jclass mcClass, playerClass, worldClass, fontRendererClass;
extern jfieldID mcInstanceField, localPlayerField, worldField, playersListField, fontRendererField;
extern jmethodID toArrayMethod, drawStringMethod;

StatCheckerModule::StatCheckerModule() : Module("StatChecker", false) {
    loadCache();
    hSession = WinHttpOpen(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
        // Set short timeouts: Resolve, Connect, Send, Receive (in ms)
        WinHttpSetTimeouts(hSession, 3000, 3000, 3000, 3000);
        hConnectHypixel = WinHttpConnect(hSession, L"api.hypixel.net", INTERNET_DEFAULT_HTTPS_PORT, 0);
        hConnectPlancke = WinHttpConnect(hSession, L"plancke.io", INTERNET_DEFAULT_HTTPS_PORT, 0);
    }
    workerThread = std::thread(&StatCheckerModule::workerLoop, this);
}

StatCheckerModule::~StatCheckerModule() {
    {
        std::lock_guard<std::mutex> l(cacheMutex);
        stopWorker = true;
    }
    queueCV.notify_all();
    if (workerThread.joinable()) workerThread.join();
    
    saveCache();

    if (hConnectHypixel) WinHttpCloseHandle(hConnectHypixel);
    if (hConnectPlancke) WinHttpCloseHandle(hConnectPlancke);
    if (hSession) WinHttpCloseHandle(hSession);
}

void StatCheckerModule::workerLoop() {
    while (true) {
        std::string name;
        {
            std::unique_lock<std::mutex> l(cacheMutex);
            queueCV.wait(l, [this] { return stopWorker || !requestQueue.empty(); });
            if (stopWorker) break;
            name = requestQueue.front();
            requestQueue.pop();
        }
        
        if (hSession) fetchStats(hSession, name);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

std::string extractJsonValue(const std::string& json, const std::string& key) {
    size_t pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return "0";
    pos += key.length() + 3;
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':')) pos++;
    size_t end;
    if (json[pos] == '"') { pos++; end = json.find('"', pos); }
    else { end = json.find_first_of(", \n\r}", pos); }
    if (end == std::string::npos) return "0";
    return json.substr(pos, end - pos);
}

void StatCheckerModule::fetchStats(void* hS, std::string name) {
    if (name == "Unknown" || name.empty() || !hConnectHypixel) return;

    std::string path = "/v2/player?key=" + apiKey + "&name=" + name;
    std::wstring wpath(path.begin(), path.end());
    HINTERNET hR = WinHttpOpenRequest(hConnectHypixel, L"GET", wpath.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    
    if (hR) {
        if (WinHttpSendRequest(hR, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hR, NULL)) {
            DWORD statusCode = 0; DWORD statusSize = sizeof(statusCode);
            WinHttpQueryHeaders(hR, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

            if (statusCode == 200) {
                std::string resp; DWORD sz = 0;
                while (WinHttpQueryDataAvailable(hR, &sz) && sz > 0) {
                    char* buf = new char[sz + 1]; DWORD rd = 0;
                    if (WinHttpReadData(hR, buf, sz, &rd)) { buf[rd] = 0; resp += buf; }
                    delete[] buf;
                }
                if (!resp.empty() && resp.find("\"player\":null") == std::string::npos) {
                    PlayerStats s;
                    try {
                        s.bwStars = std::stoi(extractJsonValue(resp, "bedwars_level"));
                        s.finalKills = std::stoi(extractJsonValue(resp, "final_kills_bedwars"));
                        s.finalDeaths = std::stoi(extractJsonValue(resp, "final_deaths_bedwars"));
                        s.fkdr = (s.finalDeaths > 0) ? (float)s.finalKills / s.finalDeaths : (float)s.finalKills;
                        std::string exStr = extractJsonValue(resp, "networkExp");
                        long long ex = (exStr != "0") ? std::stoll(exStr) : 0;
                        s.level = (int)((sqrt(2.0 * ex + 15312.5) - 125.0) / 50.0 + 1.0);
                    } catch (...) { if (s.level <= 0) s.level = 1; }
                    s.loaded = true; s.lastUpdate = GetTickCount64();
                    std::lock_guard<std::mutex> l(cacheMutex); cache[name] = s;
                } else {
                    fetchStatsPlancke(hS, name);
                }
            } else {
                fetchStatsPlancke(hS, name);
            }
        } else {
            fetchStatsPlancke(hS, name);
        }
        WinHttpCloseHandle(hR);
    }
}

void StatCheckerModule::fetchStatsPlancke(void* hS, std::string name) {
    if (!hConnectPlancke) return;
    
    std::string path = "/hypixel/player/stats/" + name;
    std::wstring wpath(path.begin(), path.end());
    HINTERNET hR = WinHttpOpenRequest(hConnectPlancke, L"GET", wpath.c_str(), NULL, L"https://plancke.io/", WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    
    if (hR) {
        // Add additional headers to look more like a real browser
        WinHttpAddRequestHeaders(hR, L"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8\r\n", -1L, WINHTTP_ADDREQ_FLAG_ADD);
        
        if (WinHttpSendRequest(hR, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hR, NULL)) {
            std::string resp; DWORD sz = 0;
            while (WinHttpQueryDataAvailable(hR, &sz) && sz > 0) {
                char* buf = new char[sz + 1]; DWORD rd = 0;
                if (WinHttpReadData(hR, buf, sz, &rd)) { buf[rd] = 0; resp += buf; }
                delete[] buf;
            }
            
            if (!resp.empty() && resp.find("stat_panel_BedWars") != std::string::npos) {
                PlayerStats s;
                
                // Parse Hypixel Level
                size_t p = resp.find("<b>Level:</b>");
                if (p != std::string::npos) {
                    p += 13; size_t end = resp.find("<", p);
                    if (end != std::string::npos) s.level = (int)std::stof(resp.substr(p, end - p));
                }
                
                // Parse Bedwars Level
                p = resp.find("id=\"stat_panel_BedWars\"");
                if (p != std::string::npos) {
                    p = resp.find("<b>Level:</b>", p);
                    if (p != std::string::npos) {
                        p += 13; size_t end = resp.find("<", p);
                        if (end != std::string::npos) s.bwStars = std::stoi(resp.substr(p, end - p));
                    }
                }
                
                // Parse Bedwars Overall Stats (FKDR)
                p = resp.find("Overall</th>");
                if (p != std::string::npos) {
                    // Skip 3 TDs to get Final Kills
                    for (int i = 0; i < 4; i++) p = resp.find("<td>", p + 4);
                    if (p != std::string::npos) {
                        p += 4; size_t end = resp.find("<", p);
                        if (end != std::string::npos) s.finalKills = std::stoi(resp.substr(p, end - p));
                        
                        p = resp.find("<td>", end);
                        if (p != std::string::npos) {
                            p += 4; end = resp.find("<", p);
                            if (end != std::string::npos) s.finalDeaths = std::stoi(resp.substr(p, end - p));
                        }
                    }
                }
                
                if (s.finalDeaths > 0) s.fkdr = (float)s.finalKills / s.finalDeaths;
                else s.fkdr = (float)s.finalKills;
                
                s.loaded = true; s.lastUpdate = GetTickCount64();
                std::lock_guard<std::mutex> l(cacheMutex); cache[name] = s;
            }
        }
        WinHttpCloseHandle(hR);
    }
}

void StatCheckerModule::saveCache() {
    std::ofstream f(PathUtils::GetPathA("GUI\\Modules\\stats_database.dat"));
    if (!f.is_open()) return;
    std::lock_guard<std::mutex> l(cacheMutex);
    for (auto const& [name, s] : cache) {
        if (!s.loaded) continue;
        f << name << "," << s.level << "," << s.bwStars << "," << s.finalKills << "," << s.finalDeaths << "," << s.lastUpdate << "\n";
    }
}

void StatCheckerModule::loadCache() {
    std::ifstream f(PathUtils::GetPathA("GUI\\Modules\\stats_database.dat"));
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        std::stringstream ss(line);
        std::string n, item;
        PlayerStats s;
        if (!std::getline(ss, n, ',')) continue;
        if (!std::getline(ss, item, ',')) continue; s.level = std::stoi(item);
        if (!std::getline(ss, item, ',')) continue; s.bwStars = std::stoi(item);
        if (!std::getline(ss, item, ',')) continue; s.finalKills = std::stoi(item);
        if (!std::getline(ss, item, ',')) continue; s.finalDeaths = std::stoi(item);
        if (!std::getline(ss, item, ',')) continue; s.lastUpdate = std::stoull(item);
        s.fkdr = (s.finalDeaths > 0) ? (float)s.finalKills / s.finalDeaths : (float)s.finalKills;
        s.loaded = true;
        cache[n] = s;
    }
}

std::string StatCheckerModule::getName(JNIEnv* env, jobject p) {
    if (!p) return "Unknown";
    jclass cls = env->GetObjectClass(p);
    jmethodID mid = env->GetMethodID(cls, "e_", "()Ljava/lang/String;");
    if (!mid) { env->ExceptionClear(); mid = env->GetMethodID(cls, "getName", "()Ljava/lang/String;"); }
    
    if (mid) {
        jstring js = (jstring)env->CallObjectMethod(p, mid);
        if (js) {
            const char* str = env->GetStringUTFChars(js, nullptr);
            std::string res(str); env->ReleaseStringUTFChars(js, str);
            env->DeleteLocalRef(js); return res;
        }
    }
    env->ExceptionClear();
    return "Unknown";
}

void StatCheckerModule::onRender2D() {
    // Left empty. Rendering is now handled by ClickGUI::render to avoid state corruption.
}

const char* getStarColor(int stars) {
    if (stars >= 1000) return "§c"; // Rainbow-ish (Red for now)
    if (stars >= 900) return "§5";  // Purple
    if (stars >= 800) return "§9";  // Blue
    if (stars >= 700) return "§f";  // White
    if (stars >= 600) return "§d";  // Pink
    if (stars >= 500) return "§4";  // Dark Red
    if (stars >= 400) return "§3";  // Dark Aqua
    if (stars >= 300) return "§2";  // Dark Green
    if (stars >= 200) return "§b";  // Aqua
    if (stars >= 100) return "§6";  // Gold
    return "§7";                   // Gray
}

void StatCheckerModule::drawStats(JNIEnv* env, jobject fr, int sw, int sh) {
    jobject mc = env->GetStaticObjectField(mcClass, mcInstanceField);
    jobject world = env->GetObjectField(mc, worldField);
    jobject lp = env->GetObjectField(mc, localPlayerField);
    if (!world || !lp) { env->DeleteLocalRef(fr); env->DeleteLocalRef(mc); return; }
    
    jobject pl = env->GetObjectField(world, playersListField);
    if (!pl) { env->DeleteLocalRef(fr); env->DeleteLocalRef(lp); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }
    jobjectArray pa = (jobjectArray)env->CallObjectMethod(pl, toArrayMethod);
    if (!pa) { env->DeleteLocalRef(pl); env->DeleteLocalRef(fr); env->DeleteLocalRef(lp); env->DeleteLocalRef(world); env->DeleteLocalRef(mc); return; }
    
    int cnt = env->GetArrayLength(pa);
    
    struct RenderEntry { std::string name; PlayerStats stats; bool isLocal; };
    std::vector<RenderEntry> entries;

    for (int i = 0; i < cnt; i++) {
        jobject p = env->GetObjectArrayElement(pa, i); if (!p) continue;
        std::string n = getName(env, p);
        bool isMe = env->IsSameObject(p, lp);
        PlayerStats s; bool f = false;
        { std::lock_guard<std::mutex> l(cacheMutex); if (cache.count(n)) { s = cache[n]; f = true; } }

        if (!f || (GetTickCount64() - s.lastUpdate > 600000)) {
            if (!s.loading) {
                s.loading = true; { std::lock_guard<std::mutex> l(cacheMutex); cache[n] = s; requestQueue.push(n); }
                queueCV.notify_one();
            }
        }
        entries.push_back({ n, s, isMe });
        env->DeleteLocalRef(p);
    }

    // Sort by FKDR (highest first), put loading ones at the bottom
    std::sort(entries.begin(), entries.end(), [](const RenderEntry& a, const RenderEntry& b) {
        if (a.stats.loaded != b.stats.loaded) return a.stats.loaded;
        return a.stats.fkdr > b.stats.fkdr;
    });

    // Make it cover almost the full screen (90% width/height)
    int lw = (int)(sw * 0.9f);
    int lh = (int)(sh * 0.9f);
    int sx = (sw - lw) / 2, sy = (sh - lh) / 2;
    int md = std::min((int)entries.size(), (lh - 60) / 16); // Dynamically calculate how many fit

    // Backgrounds
    ClickGUI::drawRect(0, 0, (float)sw, (float)sh, 0.0f, 0.0f, 0.0f, 1.0f); // Opaque takeover
    ClickGUI::drawRect((float)sx, (float)sy, (float)lw, (float)lh, 0.02f, 0.02f, 0.02f, 1.0f); // Opaque panel
    ClickGUI::drawRect((float)sx, (float)sy, (float)lw, 30.0f, 0.4f, 0.02f, 0.02f, 1.0f); // Header

    ClickGUI::drawText(env, fr, "§l§nHYPIXEL THREAT ANALYZER v2.0", sx + 15, sy + 8);
    
    // Proportional column offsets
    int colName = 20;
    int colStars = (int)(lw * 0.45f);
    int colFkdr = (int)(lw * 0.60f);
    int colLevel = (int)(lw * 0.75f);
    int colThreat = (int)(lw * 0.85f);

    int dy = sy + 40;
    ClickGUI::drawText(env, fr, "§8§lPLAYER NAME", sx + colName, dy);
    ClickGUI::drawText(env, fr, "§8§lSTARS", sx + colStars, dy);
    ClickGUI::drawText(env, fr, "§8§lFKDR", sx + colFkdr, dy);
    ClickGUI::drawText(env, fr, "§8§lLEVEL", sx + colLevel, dy);
    ClickGUI::drawText(env, fr, "§8§lTHREAT", sx + colThreat, dy);
    dy += 18;

    for (int i = 0; i < md; i++) {
        const auto& e = entries[i];
        std::string prefix = e.isLocal ? "§b" : "§f";
        
        ClickGUI::drawText(env, fr, (prefix + e.name).c_str(), sx + colName, dy);
        
        if (e.stats.loaded) {
            std::string starStr = std::string(getStarColor(e.stats.bwStars)) + "[" + std::to_string(e.stats.bwStars) + "§7*]";
            ClickGUI::drawText(env, fr, starStr.c_str(), sx + colStars, dy);

            const char* fkColor = (e.stats.fkdr > 5.0f) ? "§c" : (e.stats.fkdr > 2.0f) ? "§6" : (e.stats.fkdr > 1.0f) ? "§e" : "§a";
            std::stringstream fkss; fkss << fkColor << std::fixed << std::setprecision(2) << e.stats.fkdr;
            ClickGUI::drawText(env, fr, fkss.str().c_str(), sx + colFkdr, dy);

            ClickGUI::drawText(env, fr, ("§7" + std::to_string(e.stats.level)).c_str(), sx + colLevel, dy);

            if (e.stats.fkdr > 5.0f) ClickGUI::drawText(env, fr, "§l§c[ULTRA]", sx + colThreat, dy);
            else if (e.stats.fkdr > 2.0f) ClickGUI::drawText(env, fr, "§l§6[SWEAT]", sx + colThreat, dy);
            else if (e.stats.fkdr > 1.0f) ClickGUI::drawText(env, fr, "§e[DECENT]", sx + colThreat, dy);
            else ClickGUI::drawText(env, fr, "§a[EASY]", sx + colThreat, dy);
        } else {
            ClickGUI::drawText(env, fr, "§8Scanning...", sx + colStars, dy);
        }
        
        dy += 16;
    }

    env->DeleteLocalRef(pa); env->DeleteLocalRef(pl); env->DeleteLocalRef(lp); env->DeleteLocalRef(world); env->DeleteLocalRef(mc);
}
