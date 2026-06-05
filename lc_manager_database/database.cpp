#include "database.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "../utils.h"

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
            abno.code = val.value("code", "");
            abno.name = NormalizeText(val.value("name", ""));
            abno.riskLevel = val.value("risk", "");

            if (val.contains("wiki_url")) {
                abno.wikiLink = val["wiki_url"];
            } else {
                abno.wikiLink = "";
            }

            abnormalities[abno.id] = abno;
        }

        std::ifstream equipFile(equipPath);
        json equipJson = json::parse(equipFile);

        for (auto& [key, val] : equipJson.items()) {
            Equipment eq;
            eq.id = std::stoi(key);
            eq.type = val.value("type", "");
            eq.name = NormalizeText(val.value("name", ""));
            eq.hasBonus = val.value("bonus", false);

            if (val.contains("stats")) {
                eq.stats.hp = val["stats"].value("hp", 0);
                eq.stats.sp = val["stats"].value("sp", 0);
                eq.stats.ws = val["stats"].value("ws", 0);
                eq.stats.as = val["stats"].value("as", 0);
            }

            eq.abnormalityName = NormalizeText(val.value("abnormality_name", ""));
            eq.abnormalityId = val.value("abnormality_id", "");
            eq.dmgType = val.value("dmg_type", "");
            eq.dmg = val.value("dmg", "");
            eq.attackSpeed = val.value("attack_speed", "");
            eq.range = val.value("range", "");
            eq.weaponType = val.value("weapon_type", "");
            eq.defenseRed = val.value("defense_RED", "");
            eq.defenseWhite = val.value("defense_WHITE", "");
            eq.defenseBlack = val.value("defense_BLACK", "");
            eq.defensePale = val.value("defense_PALE", "");
            eq.attachmentSlot = val.value("attachment_slot", "");

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
