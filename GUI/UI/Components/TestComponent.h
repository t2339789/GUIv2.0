#pragma once
#include "../Core/UIComponent.h"
#include "../Core/Canvas.h"

class TestComponent : public UIComponent {
public:
    void onUpdate() override {
        // Simple state update for testing
    }

    void onDraw(Canvas& canvas) override {
    }
};
