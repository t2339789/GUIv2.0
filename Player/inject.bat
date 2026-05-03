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

:: 3. Find JDK for compilation
echo [+] Locating JDK headers...
set "JDK_PATH="
if defined JAVA_HOME (
    if exist "!JAVA_HOME!\include" set "JDK_PATH=!JAVA_HOME!"
)

if "!JDK_PATH!"=="" (
    for /d %%d in ("C:\Program Files\Java\jdk-*") do (
        if exist "%%d\include" set "JDK_PATH=%%d"
    )
)

if "!JDK_PATH!"=="" (
    echo [^!] ERROR: JDK not found. Please set JAVA_HOME or install JDK.
    echo [^!] Expected JDK headers in C:\Program Files\Java\jdk-*
    pause
    exit /b
)
echo [+] Found JDK: !JDK_PATH!

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
set "BASE_DIR=%~dp0"
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
    -I"!JDK_PATH!\include" -I"!JDK_PATH!\include\win32" ^
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
