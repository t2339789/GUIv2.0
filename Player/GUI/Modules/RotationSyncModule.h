#pragma once
#include "../Module.h"

class RotationSyncModule : public Module {
private:
    bool wasLMBPressed;

public:
    RotationSyncModule();
    virtual void onRender2D() override;
};
