#include "ModuleManager.h"
#include "Modules/ESPModule.h"
#include "Modules/AimAssistModule.h"
#include "Modules/VelocityModule.h"
#include "Modules/HitRegModule.h"
#include "Modules/StatCheckerModule.h"
#include "Modules/RotationSyncModule.h"
#include "Modules/ComboBreakerModule.h"
#include "Modules/StrafeAssistModule.h"

std::vector<Module*> ModuleManager::modules;

void ModuleManager::init() {
    modules.push_back(new ESPModule());
    modules.push_back(new AimAssistModule());
    modules.push_back(new VelocityModule());
    modules.push_back(new HitRegModule());
    modules.push_back(new StatCheckerModule());
    modules.push_back(new RotationSyncModule());
    modules.push_back(new ComboBreakerModule());
    modules.push_back(new StrafeAssistModule());
}

void ModuleManager::cleanup() {
    for (Module* mod : modules) {
        delete mod;
    }
    modules.clear();
}

void ModuleManager::render3D() {
    for (Module* mod : modules) {
        if (mod->isEnabled()) {
            mod->onRender();
        }
    }
}

void ModuleManager::render2D() {
    for (Module* mod : modules) {
        if (mod->isEnabled()) {
            mod->onRender2D();
        }
    }
}

std::vector<Module*>& ModuleManager::getModules() {
    return modules;
}

