#pragma once
#include <string>
#include <vector>
#include <map>

struct Abnormality {
    int id;
    std::string code;
    std::string name;
    std::string riskLevel;
    std::string wikiLink;
};

struct EquipmentStats {
    int hp;
    int sp;
    int ws;
    int as;
};

struct Equipment {
    int id;
    std::string type;
    std::string name;
    bool hasBonus;
    EquipmentStats stats;
    std::string abnormalityName;
    std::string abnormalityId;
    std::string dmgType;
    std::string dmg;
    std::string attackSpeed;
    std::string range;
    std::string weaponType;
    std::string defenseRed;
    std::string defenseWhite;
    std::string defenseBlack;
    std::string defensePale;
    std::string attachmentSlot;
};

class DatabaseManager {
private:
    std::map<int, Abnormality> abnormalities;
    std::map<int, Equipment> equipments;

public:
    bool Initialize(const std::string& dbFolder);

    Abnormality* GetAbnormality(int id);
    Abnormality* GetAbnormalityByCode(const std::string& code);
    Equipment* GetEquipment(int id);
    size_t GetAbnormalityCount() const;
    size_t GetEquipmentCount() const;
};
