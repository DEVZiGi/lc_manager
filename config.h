#pragma once

#include <string>

struct AppSettings {
    bool autoInject = false;
    bool superFastForward = false;
    float tt15Speed = 5.0f;
    float tt2Speed = 10.0f;
    double totalPlayTime = 0.0;
};

void LoadConfig(AppSettings& settings, const std::string& configPath);
void SaveConfig(const AppSettings& settings, const std::string& configPath);
