#pragma once
#include <vector>
#include "Module.h"


class ModuleManager {
private:
    static std::vector<Module*> modules;
public:
    static void init();
    static void cleanup();
    static void render3D();
    static void render2D();
    static std::vector<Module*>& getModules();
};
