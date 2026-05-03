#pragma once
#include "../Module.h"
#include <jni.h>

class HitRegModule : public Module {
public:
    HitRegModule();
    virtual void onRender2D() override;
};
