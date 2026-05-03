#pragma once
#include <string>
#include <vector>
#include <functional>

// A single configurable setting
struct Setting {
    enum Type { FLOAT_SLIDER, BOOL_TOGGLE };
    Type type;
    std::string name;
    float value;
    float minVal, maxVal;  // Only for FLOAT_SLIDER

    // Float slider constructor
    Setting(const std::string& name, float val, float min, float max)
        : type(FLOAT_SLIDER), name(name), value(val), minVal(min), maxVal(max) {}

    // Bool toggle constructor
    Setting(const std::string& name, bool val)
        : type(BOOL_TOGGLE), name(name), value(val ? 1.0f : 0.0f), minVal(0), maxVal(1) {}

    bool getBool() const { return value > 0.5f; }
    void setBool(bool b) { value = b ? 1.0f : 0.0f; }
};

class Module {
protected:
    std::string name;
    bool enabled;
    std::vector<Setting> settings;

    // Helper for subclasses to register settings
    void addSetting(const std::string& name, float val, float min, float max) {
        settings.push_back(Setting(name, val, min, max));
    }
    void addSetting(const std::string& name, bool val) {
        settings.push_back(Setting(name, val));
    }

public:
    Module(const std::string& name, bool enabled = false) : name(name), enabled(enabled) {}
    virtual ~Module() {}

    virtual void onEnable() {}
    virtual void onDisable() {}
    virtual void onRender() {}   // For 3D world rendering
    virtual void onRender2D() {} // For 2D HUD rendering

    std::string getName() const { return name; }
    bool isEnabled() const { return enabled; }
    void toggle() {
        enabled = !enabled;
        if (enabled) onEnable();
        else onDisable();
    }
    void setEnabled(bool state) {
        if (enabled != state) {
            enabled = state;
            if (enabled) onEnable();
            else onDisable();
        }
    }

    // Settings access
    std::vector<Setting>& getSettings() { return settings; }
    Setting* getSetting(const std::string& name) {
        for (auto& s : settings) {
            if (s.name == name) return &s;
        }
        return nullptr;
    }
};
