@echo off
setlocal enabledelayedexpansion
TITLE Poltergeist: Standalone EXE Builder
echo ========================================
echo   Poltergeist Standalone EXE Builder
echo ========================================

set "BASE_DIR=%~dp0"
set "INCLUDE_DIR=%BASE_DIR%ESP\include"

:: 1. Ensure JNI/JVMTI headers exist
if not exist "%INCLUDE_DIR%\jni.h" (
    echo [+] Missing JDK headers. Attempting automatic fix...
    
    :: Try to find JDK first
    set "JDK_PATH="
    if defined JAVA_HOME if exist "!JAVA_HOME!\include" set "JDK_PATH=!JAVA_HOME!"
    if "!JDK_PATH!"=="" (
        for /d %%d in ("C:\Program Files\Java\jdk-*") do (
            if exist "%%d\include" set "JDK_PATH=%%d"
        )
    )
    
    if defined JDK_PATH (
        echo [+] Copying headers from !JDK_PATH!...
        copy "!JDK_PATH!\include\jni.h" "%INCLUDE_DIR%\" >nul
        copy "!JDK_PATH!\include\win32\jni_md.h" "%INCLUDE_DIR%\" >nul
        copy "!JDK_PATH!\include\jvmti.h" "%INCLUDE_DIR%\" >nul
    ) else (
        echo [+] Downloading headers from public repository...
        powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/openjdk/jdk/master/src/java.base/share/native/include/jni.h' -OutFile '%INCLUDE_DIR%\jni.h'"
        powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/openjdk/jdk/master/src/java.base/windows/native/include/jni_md.h' -OutFile '%INCLUDE_DIR%\jni_md.h'"
        powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/openjdk/jdk/master/src/java.base/share/native/include/jvmti.h' -OutFile '%INCLUDE_DIR%\jvmti.h'"
    )
)

:: 2. Compile DLL
echo [+] Compiling ESP.dll...
cd /d "%BASE_DIR%ESP"

:: We include "include" which now contains jni.h etc.
g++ -shared -o ESP.dll ESPDLL.cpp ^
    ../GUI/ModuleManager.cpp ^
    ../GUI/ClickGUI.cpp ^
    ../GUI/Modules/AimAssistModule.cpp ^
    ../GUI/Modules/ComboBreakerModule.cpp ^
    ../GUI/Modules/ESPModule.cpp ^
    ../GUI/Modules/HitRegModule.cpp ^
    ../GUI/Modules/RotationSyncModule.cpp ^
    ../GUI/Modules/StatCheckerModule.cpp ^
    ../GUI/Modules/StrafeAssistModule.cpp ^
    ../GUI/Modules/VelocityModule.cpp ^
    src/buffer.c src/hook.c src/trampoline.c ^
    src/hde/hde32.c src/hde/hde64.c ^
    -I"include" -I"../GUI" -I"../GUI/Modules" ^
    -lgdi32 -lgdiplus -lopengl32 -lwinhttp -static-libgcc -static-libstdc++

if errorlevel 1 (
    echo [!] ERROR: DLL Compilation failed. Check if MinGW/g++ is installed.
    pause
    exit /b
)

:: 3. Convert DLL to Header
echo [+] Embedding DLL into header...
python dll_to_header.py ESP.dll DllData.h

if errorlevel 1 (
    echo [!] ERROR: Embedding failed. Check if Python is installed.
    pause
    exit /b
)

:: 4. Compile Standalone Injector
echo [+] Compiling Standalone Poltergeist.exe...
g++ -o ../Poltergeist.exe Injector.cpp -static-libgcc -static-libstdc++

if errorlevel 1 (
    echo [!] ERROR: EXE Compilation failed.
    pause
    exit /b
)

:: 5. Cleanup
echo [+] Cleaning up temporary files...
del ESP.dll
del DllData.h

echo ========================================
echo [SUCCESS] Standalone EXE created: Poltergeist.exe
echo You can now send this file to anyone!
echo ========================================
pause
