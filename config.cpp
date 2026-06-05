#include "config.h"
#include "utils.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

void LoadConfig(AppSettings& settings, const std::string& configPath) {
    try {
        std::ifstream file(configPath);
        if (file.is_open()) {
            json j;
            file >> j;
            settings.autoInject = j.value("autoInject", false);
            settings.superFastForward = j.value("superFastForward", false);
            settings.tt15Speed = j.value("tt15Speed", 5.0f);
            settings.tt2Speed = j.value("tt2Speed", 10.0f);
            settings.totalPlayTime = j.value("totalPlayTime", 0.0);
        }
    } catch (...) {}
}

void SaveConfig(const AppSettings& settings, const std::string& configPath) {
    try {
        json j;
        j["autoInject"] = settings.autoInject;
        j["superFastForward"] = settings.superFastForward;
        j["tt15Speed"] = settings.tt15Speed;
        j["tt2Speed"] = settings.tt2Speed;
        j["totalPlayTime"] = settings.totalPlayTime;
        std::ofstream file(configPath);
        file << j.dump(4);
        
        j["targetDir"] = fs::current_path().string() + "\\lc_manager_database";
        fs::create_directories(j["targetDir"]);
        
        std::string dataPath = GetLocalLowPath() + "\\data";
        fs::create_directories(dataPath);
        
        std::string commandsPath = dataPath + "\\commands.json";
        std::ofstream cmdFile(commandsPath);
        cmdFile << j.dump(4);
    } catch (...) {}
}
