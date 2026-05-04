# GUI/UI Rendering Wrapper

This folder now includes a small immediate-mode wrapper to make drawing UI fast and consistent:

- `Core/Canvas.h`: minimal backend interface (`drawRect`, `drawText`)
- `Core/SkiaCanvas.h`: OpenGL-backed implementation
- `Core/ScreenUI.h`: high-level primitives for panel/module/settings rendering
  - Includes switch-style binary rows and segmented-control primitive

## Quick Start

```cpp
#include "UI/Core/ScreenUI.h"

void MyComponent::onDraw(Canvas& canvas) {
    ScreenUI ui(canvas);
    ui.beginPanel(40.0f, 40.0f, 260.0f, 360.0f, "Poltergeist");

    ui.rowModule("ESP", true, false, true, false);
    ui.beginSettingsBlock(2);
    ui.rowSliderLabel("Range", 4.0f);
    ui.rowSliderTrack(4.0f, 1.0f, 6.0f);
    ui.nextRow();
    ui.rowToggle("Require Click", true); // immediate binary switch
    ui.nextRow();
}
```

For 3+ mutually exclusive modes, use a segmented control primitive instead of multiple toggles:

```cpp
const char* opts[] = { "Off", "Hold", "Toggle" };
ui.rowSegmented("Activation", opts, 3, 1);
ui.nextRow();
```

## Input Model

For interactive components, keep your own per-frame input state:

- mouse position
- `lmbDown`
- `clicked` edge
- `released` edge
- optional right-click edge for expand/collapse

`ModulePanelComponent` is a full reference implementation for:

- panel dragging
- module toggle click
- settings expand/collapse
- slider drag
- toggle click

## Compatibility with Modules

`ModulePanelComponent` reads from `ModuleManager::getModules()` and mutates each module's live `Setting` objects directly. That keeps rendering and behavior fully compatible with existing modules.

## Runtime UI Switch

In the hook path (`ESP/ESPDLL.cpp`):

- `Ctrl+9` toggles old/new UI renderer path
- `Insert` and `Ctrl+0` toggle visibility for the new renderer path

This allows A/B testing while migrating off `ClickGUI`.
