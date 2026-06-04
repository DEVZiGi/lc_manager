#include "database.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using json = nlohmann::json;

bool DatabaseManager::Initialize(const std::string& dbFolder) {
    std::string abnoPath = dbFolder + "\\abno_db.json";
    std::string equipPath = dbFolder + "\\equip_db.json";

    if (!fs::exists(abnoPath) || !fs::exists(equipPath)) {
        return false;
    }

    try {
        std::ifstream abnoFile(abnoPath);
        json abnoJson = json::parse(abnoFile);

        for (auto& [key, val] : abnoJson.items()) {
            Abnormality abno;
            abno.id = std::stoi(key);
            abno.code = val["code"];
            abno.name = val["name"];
            abno.riskLevel = val["risk"];

            std::string wikiName = abno.name;
            std::replace(wikiName.begin(), wikiName.end(), ' ', '_');
            abno.wikiLink = "https://lobotomycorp.fandom.com/wiki/" + wikiName;

            abnormalities[abno.id] = abno;
        }

        std::ifstream equipFile(equipPath);
        json equipJson = json::parse(equipFile);

        for (auto& [key, val] : equipJson.items()) {
            Equipment eq;
            eq.id = std::stoi(key);
            eq.type = val["type"];
            eq.name = val["name"];
            eq.hasBonus = val["bonus"];

            eq.stats.hp = val["stats"]["hp"];
            eq.stats.sp = val["stats"]["sp"];
            eq.stats.ws = val["stats"]["ws"];
            eq.stats.as = val["stats"]["as"];

            equipments[eq.id] = eq;
        }

        return true;
    } catch (...) {
        return false;
    }
}

Abnormality* DatabaseManager::GetAbnormality(int id) {
    auto it = abnormalities.find(id);
    if (it != abnormalities.end()) return &it->second;
    return nullptr;
}

Abnormality* DatabaseManager::GetAbnormalityByCode(const std::string& code) {
    for (auto& pair : abnormalities) {
        if (pair.second.code == code) return &pair.second;
    }
    return nullptr;
}

Equipment* DatabaseManager::GetEquipment(int id) {
    auto it = equipments.find(id);
    if (it != equipments.end()) return &it->second;
    return nullptr;
}

size_t DatabaseManager::GetAbnormalityCount() const {
    return abnormalities.size();
}

size_t DatabaseManager::GetEquipmentCount() const {
    return equipments.size();
}
