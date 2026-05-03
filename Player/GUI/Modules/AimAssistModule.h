#pragma once
#include "../Module.h"
#include <jni.h>

#include <vector>

class AimAssistModule : public Module {
private:
    std::vector<unsigned long long> clicks;
    bool wasLMBPressed;
    float lastYaw, lastPitch;
    float smoothedYaw, smoothedPitch;

public:
    AimAssistModule();
    virtual void onRender2D() override;
};
