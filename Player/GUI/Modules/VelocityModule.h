#pragma once
#include "../Module.h"
#include <jni.h>

class VelocityModule : public Module {
private:
    float targetYaw;
    bool needsRotation;

public:
    VelocityModule();
    virtual void onRender2D() override;
};
