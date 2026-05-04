#pragma once
#include "../Module.h"

class DiscreetFlyModule : public Module {
public:
    DiscreetFlyModule();
    void onRender2D() override;

private:
    unsigned long long lastAntiKickPulse = 0;
};
