#pragma once
#include "Canvas.h"

// Base class for all reactive UI components
class UIComponent {
protected:
    bool visible = true;
public:
    virtual ~UIComponent() {}
    virtual void onUpdate() = 0; // Logic/State updates
    virtual void onDraw(Canvas& canvas) = 0; // Rendering
    
    void setVisible(bool v) { visible = v; }
    bool isVisible() const { return visible; }
};
