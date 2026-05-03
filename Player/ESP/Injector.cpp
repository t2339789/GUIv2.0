#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

// If we are building the standalone version, this file will be generated
#if __has_include("DllData.h")
#include "DllData.h"
#define STANDALONE
#endif

DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (_wcsicmp(entry.szExeFile, processName) == 0) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

bool InjectDLL(DWORD pid, const std::wstring& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::wcerr << L"Failed to open process: " << GetLastError() << std::endl;
        return false;
    }

    LPVOID remoteBuf = VirtualAllocEx(hProcess, NULL, (dllPath.size() + 1) * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuf) {
        std::wcerr << L"Failed to allocate memory in remote process" << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteBuf, dllPath.c_str(), (dllPath.size() + 1) * sizeof(wchar_t), NULL)) {
        std::wcerr << L"Failed to write memory in remote process" << std::endl;
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remoteBuf, 0, NULL);
    if (!hThread) {
        std::wcerr << L"Failed to create remote thread" << std::endl;
        VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    VirtualFreeEx(hProcess, remoteBuf, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return exitCode != 0;
}

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"   Poltergeist Standalone Injector" << std::endl;
    std::wcout << L"========================================" << std::endl;

    DWORD pid = GetProcessIdByName(L"javaw.exe");
    if (pid == 0) {
        std::wcerr << L"[!] ERROR: javaw.exe (Minecraft) not found." << std::endl;
        std::wcerr << L"[!] Please launch the game first." << std::endl;
        system("pause");
        return 1;
    }

    std::wcout << L"[+] Found Minecraft (PID: " << pid << L")" << std::endl;

    std::wstring dllPath;

#ifdef STANDALONE
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring targetDll = std::wstring(tempPath) + L"Poltergeist_ESP.dll";
    
    std::wcout << L"[+] Extracting embedded DLL..." << std::endl;
    std::ofstream out(targetDll, std::ios::binary);
    out.write((const char*)rawDll, sizeof(rawDll));
    out.close();
    
    dllPath = targetDll;
#else
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring exePath(buffer);
    std::wstring baseDir = exePath.substr(0, exePath.find_last_of(L"\\/"));
    dllPath = baseDir + L"\\ESP.dll";

    if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        dllPath = baseDir + L"\\ESP\\ESP.dll";
        if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
             std::wcerr << L"[!] ERROR: ESP.dll not found." << std::endl;
             system("pause");
             return 1;
        }
    }
#endif

    std::wcout << L"[+] Injecting..." << std::endl;

    if (InjectDLL(pid, dllPath)) {
        std::wcout << L"[SUCCESS] Poltergeist is now active." << std::endl;
    } else {
        std::wcerr << L"[FAILURE] Injection failed." << std::endl;
    }

#ifdef STANDALONE
    // We can't delete it immediately as it's loaded, but we can try
    DeleteFileW(dllPath.c_str());
#endif

    std::wcout << L"========================================" << std::endl;
    system("pause");
    return 0;
}
