#include "UI/Core/SkiaRenderer.h"
#include "UI/Core/HtmlCanvas.h"
#include "UI/Components/ModulePanelComponent.h"
#include <windows.h>
#include <memory>

static std::unique_ptr<SkiaRenderer> g_skiaRenderer;
static std::unique_ptr<ModulePanelComponent> g_panelComponent;
static bool g_skiaInitialized = false;

void InitSkia(int w, int h) {
    if (w <= 0 || h <= 0) return;

    g_skiaRenderer = std::make_unique<SkiaRenderer>(w, h);
    if (!g_skiaRenderer->isValid()) {
        g_skiaRenderer.reset();
        g_panelComponent.reset();
        g_skiaInitialized = false;
        return;
    }

    g_panelComponent = std::make_unique<ModulePanelComponent>();
    g_skiaInitialized = true;
}

void RenderSkia(HDC hDc, int w, int h) {
    if (!g_skiaInitialized || !g_skiaRenderer ||
        g_skiaRenderer->getWidth() != w || g_skiaRenderer->getHeight() != h) {
        InitSkia(w, h);
    }

    if (!g_skiaInitialized || !g_skiaRenderer || !g_panelComponent) return;

    HWND hwnd = WindowFromDC(hDc);
    if (!hwnd) return;
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);

    static bool wasLmb = false;
    static bool wasRmb = false;
    const bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    const bool rmb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    const bool click = lmb && !wasLmb;
    const bool rclick = rmb && !wasRmb;
    g_panelComponent->setFrameInput(p.x, p.y, lmb, click, rclick);
    g_panelComponent->handleInput(p.x, p.y, lmb, click, !lmb);
    wasLmb = lmb;
    wasRmb = rmb;

    g_skiaRenderer->begin();
    HtmlCanvas canvasWrapper(w, h);
    g_panelComponent->onUpdate();
    if (g_panelComponent->isVisible()) {
        g_panelComponent->onDraw(canvasWrapper);
    }
    canvasWrapper.flushToOpenGL();

    g_skiaRenderer->flush();
}
