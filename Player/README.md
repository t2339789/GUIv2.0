# Poltergeist: Advanced Minecraft Stealth Overlay Architecture

This document outlines the core technical principles learned during the development of the Poltergeist internal client, focusing on JNI/JVMTI integration, OpenGL state management, and stealthy data retrieval.

## 1. Injection & JVM Integration
To interact with Minecraft (a Java application) from a C++ DLL, we utilize a dual-layer approach:
*   **JNI (Java Native Interface)**: Used for high-frequency data access (getting player positions, health, etc.). We retrieve the `JavaVM` via `JNI_GetCreatedJavaVMs` and attach the current thread to get a `JNIEnv` pointer.
*   **JVMTI (JVM Tool Interface)**: Crucial for **Obfuscation Resilience**. Instead of hardcoding class names that change every version, we use `GetLoadedClasses` and `GetClassSignature` to find the correct classes (e.g., `ave` for Minecraft 1.8.9) at runtime.

## 2. Rendering & OpenGL Synchronization
Drawing an overlay inside a game engine requires careful state management to avoid flickering or "J-character" corruption:
*   **The SwapBuffers Hook**: We hook `wglSwapBuffers` (opengl32.dll) to ensure our rendering happens at the very end of the game's frame cycle.
*   **State Isolation**: We use `glPushAttrib(GL_ALL_ATTRIB_BITS)` and `glPushMatrix()` to save the game's OpenGL state. 
*   **The Texture0 Fix**: Minecraft often leaves a non-zero texture unit active. We must manually call `glActiveTexture(GL_TEXTURE0)` before drawing text to prevent the game's font renderer from binding to the wrong unit, which causes visual artifacts.

## 3. Stealthy Data Manipulation
Retrieving stats on Hypixel requires a "Low-Profile" network strategy:
*   **Dual-Fetch Architecture**: We use the official Hypixel API as a primary source. If the API key is rate-limited or fails, we automatically fall back to **Plancke.io Scraping**.
*   **WinHttp Persistent Connections**: To avoid the "stalling" common in basic network code, we reuse `HINTERNET` handles. This eliminates the heavy SSL handshake overhead for every player, making the UI feel instantaneous.
*   **Fingerprint Spoofing**: We use high-quality browser `User-Agent` strings and `Referer` headers to ensure our Plancke scraper is indistinguishable from a legitimate Chrome user.

## 4. Anti-Cheat Avoidance (Stealth)
*   **Non-Invasive UI**: Instead of modifying game fields like `header` or `footer` (which Anti-Cheats can scan for via reflection), we draw a **Raw OpenGL Overlay** that is invisible to the game's internal logic.
*   **Rate Limiting**: Our background worker thread processes requests at a steady 100ms interval (10 requests/sec). This mirrors the behavior of a fast human browser and stays under the radar of automated traffic analysis.

## 5. Graceful Uninjection (Hot-Reloading)
To avoid restarting the game for every update, we implement a "Clean Exit" routine:
1.  **Stop Threads**: Signal background workers (like the Stat Checker) to stop and `join()` them.
2.  **Delete GlobalRefs**: All `jclass` and `jobject` references must be deleted via `env->DeleteGlobalRef()`. Failing to do this causes a memory leak and prevents the JVM from unloading our classes.
3.  **Unhook & Detach**: Use `MH_DisableHook` to restore original game functions before calling `FreeLibraryAndExitThread`.

---
*Developed for Poltergeist — The Invisible Advantage.*
