@echo off
setlocal enabledelayedexpansion
TITLE Poltergeist: Advanced Stealth Injector
echo ========================================
echo   Poltergeist Portable Setup ^& Injector
echo ========================================

:: 1. Check for Python
echo [+] Checking Python...
python --version >nul 2>&1
if errorlevel 1 (
    echo [^!] ERROR: Python is not installed or not in PATH.
    echo [^!] Please install Python from https://www.python.org/
    pause
    exit /b
)

:: 2. Install Python dependencies
echo [+] Ensuring Python dependencies (pyinjector)...
python -m pip install pyinjector --quiet
if errorlevel 1 (
    echo [^!] WARNING: Failed to install pyinjector via pip. 
    echo [^!] Trying to continue anyway...
)

set "BASE_DIR=%~dp0"
set "INCLUDE_DIR=%BASE_DIR%ESP\include"

:: 3. Ensure JNI/JVMTI headers exist
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

:: 4. Find PID of javaw.exe
echo [+] Searching for Minecraft (javaw.exe)...
set "PID="
for /f "tokens=2" %%a in ('tasklist /nh /fi "imagename eq javaw.exe" 2^>nul') do (
    set "PID=%%a"
)

if "!PID!"=="" goto :MC_NOT_FOUND

:: Validate PID is numeric
echo !PID!| findstr /r "^[0-9]*$" >nul
if errorlevel 1 goto :MC_NOT_FOUND

echo [+] Found Minecraft PID: !PID!
goto :MC_FOUND

:MC_NOT_FOUND
echo [^!] ERROR: Minecraft (javaw.exe) not found.
echo [^!] Please launch the game first.
pause
exit /b

:MC_FOUND

:: 5. Compile ESP.dll with relative paths
echo [+] Recompiling ESP.dll...
cd /d "%BASE_DIR%ESP"

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
    echo [^!] ERROR: Compilation failed. Check if MinGW/g++ is installed.
    pause
    exit /b
)

:: 6. Perform Injection
echo [+] Injecting DLL into Minecraft...
cd /d "%BASE_DIR%"
python -c "from pyinjector import inject; inject(!PID!, r'%BASE_DIR%ESP\ESP.dll')"

if errorlevel 1 (
    echo [^!] ERROR: Injection failed.
) else (
    echo [DONE] Poltergeist is now active.
)

echo ========================================
pause
