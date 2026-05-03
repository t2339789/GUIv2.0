#pragma once
#include <windows.h>
#include <string>

namespace PathUtils {
    inline std::wstring GetDllDir() {
        wchar_t path[MAX_PATH];
        HMODULE hModule = NULL;
        // Use the address of a function in this DLL to get its module handle
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)&GetDllDir, &hModule);
        GetModuleFileNameW(hModule, path, MAX_PATH);
        std::wstring ws(path);
        size_t pos = ws.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            return ws.substr(0, pos);
        }
        return L".";
    }

    inline std::wstring GetProjectDir() {
        std::wstring dllDir = GetDllDir();
        size_t pos = dllDir.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            return dllDir.substr(0, pos);
        }
        return dllDir;
    }

    inline std::wstring GetPath(const std::wstring& relativePath) {
        return GetProjectDir() + L"\\" + relativePath;
    }

    inline std::string GetPathA(const std::string& relativePath) {
        std::wstring projectDir = GetProjectDir();
        std::string projectDirA(projectDir.begin(), projectDir.end());
        return projectDirA + "\\" + relativePath;
    }
}
