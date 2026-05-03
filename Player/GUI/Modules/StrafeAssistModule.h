#pragma once
#include "../Module.h"

class StrafeAssistModule : public Module {
private:
    float lastStrafeDir;
    unsigned long long lastSwitchTime;

public:
    StrafeAssistModule();
    virtual void onRender2D() override;
};
