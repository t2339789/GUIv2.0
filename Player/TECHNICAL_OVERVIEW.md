# Poltergeist Technical Documentation

Poltergeist is an advanced internal client for Minecraft 1.8.9, written in C++ and designed to interface directly with the Java Virtual Machine (JVM) using the Java Native Interface (JNI).

---

## 1. The Injection Pipeline
The injection process is handled by `inject.bat`, which orchestrates the compilation and deployment:
1.  **Compilation**: `g++` compiles the source files into a 64-bit `ESP.dll`. It links against `opengl32`, `gdi32`, and the Java Development Kit (JDK) headers.
2.  **Discovery**: The batch script uses `tasklist` to find the Process ID (PID) of `javaw.exe`.
3.  **Injection**: A Python bridge using `pyinjector` performs the load-library injection, forcing the game to load our DLL into its own memory space.

## 2. The Native Bridge (JNI & JVMTI)
Once the DLL is loaded, `DllMain` creates a new thread to initialize the bridge:
-   **JVM Discovery**: It uses `JNI_GetCreatedJavaVMs` to find the active game instance.
-   **JVMTI Helper**: Since many clients (like Badlion) use custom classloaders, standard `env->FindClass` often fails. We use a custom `FindClassJVMTI` function that iterates through every loaded class in the entire JVM heap to find the target classes (e.g., `ave` for Minecraft, `bew` for the local player).
-   **Global References**: Critical classes and method IDs are cached as global references to ensure high-performance access during the render loop.

## 3. Rendering Engine (OpenGL Hooking)
The overlay is rendered by intercepting the game's graphics pipeline:
-   **MinHook**: We use the MinHook library to hook `wglSwapBuffers` inside `opengl32.dll`.
-   **SwapBuffers Hook**: Every time the game finishes drawing a frame and prepares to show it on screen, our `hwglSwapBuffers` function is called first.
-   **Rendering Layers**:
    -   **Render3D**: Draws world-space objects like ESP boxes and 3D health bars. It manually calculates the view-projection matrices to match the game's camera.
    -   **ClickGUI**: Draws the 2D interactive menu using an Orthographic projection (`glOrtho`) mapped to the window resolution.
    -   **Render2D**: Handles HUD elements and stat overlays.

## 4. Module Framework
All features are built on a modular inheritance system:
-   **Module Class**: The base class providing `onEnable`, `onDisable`, and `onUpdate` hooks.
-   **ModuleManager**: A singleton that manages the lifecycle of all modules, handling their registration and dispatching render calls.
-   **Settings System**: A dynamic settings API allowing modules to expose Sliders and Toggles to the ClickGUI without hardcoding UI logic.

## 5. Core Feature Logic
### ESP (Extra-Sensory Perception)
The ESP module iterates through the `world.playerEntities` list. It performs interpolation between the `lastTickPos` and `currentPos` using the game's `partialTicks` to ensure perfectly smooth 144Hz+ movement, even if the game's internal TPS is low.

### AimAssist
Calculates the yaw and pitch difference between the player and the nearest target. It uses a "Smoothing" algorithm to apply subtle mouse movements, making the assist look human-like to anti-cheat spectators.

### Velocity
Hooks the player's motion fields. When the game attempts to apply knockback, the module intercepts the packet or the motion update and scales the horizontal/vertical force based on the user's settings (e.g., 0% for "Vertical Velocity").

---

## 6. Project Directory Structure
-   `/ESP/`: Contains `ESPDLL.cpp` (the core hook and JNI mapping logic).
-   `/GUI/`: Contains `ClickGUI.cpp` and the UI rendering system.
-   `/GUI/Modules/`: The source code for every individual cheat feature.
-   `/include/`: Native library headers (MinHook, JNI, GLFW).

---
*Documentation generated on May 2nd, 2026.*
