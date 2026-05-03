#pragma once
#include <windows.h>
#include "../Module.h"

class ComboBreakerModule : public Module {
private:
    int hitsReceived;
    int hitsDealt;
    int lastMyHurtTime;
    int lastTargetHurtTime;
    unsigned long long lastHitTime;
    unsigned long long strafeEndTime;
    BYTE activeStrafeKey;

public:
    ComboBreakerModule();
    virtual void onRender2D() override;
};
