#include "ui_tabs.h"
#include "ui_components.h"
#include "utils.h"
#include "lc_manager_database/database.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <set>
#include <cmath>

static void AppendArrayRefs(const json& data, const char* key, std::vector<const json*>& refs) {
    auto it = data.find(key);
    if (it == data.end() || !it->is_array()) return;

    for (const auto& item : *it) {
        if (item.is_object()) refs.push_back(&item);
    }
}

static std::vector<const json*> CollectAgents(const json& data, bool workingOnly = false) {
    std::vector<const json*> agents;
    AppendArrayRefs(data, "agents", agents);
    if (!workingOnly) {
        AppendArrayRefs(data, "spareAgents", agents);
    }

    std::sort(agents.begin(), agents.end(), [](const json* left, const json* right) {
        bool leftWorking = JsonToString(*left, {"deployment"}, "") != "spare";
        bool rightWorking = JsonToString(*right, {"deployment"}, "") != "spare";
        if (leftWorking != rightWorking) return leftWorking > rightWorking;

        std::string leftDept = JsonToString(*left, {"currentSefira", "sefira", "department"}, "");
        std::string rightDept = JsonToString(*right, {"currentSefira", "sefira", "department"}, "");
        if (leftDept != rightDept) return leftDept < rightDept;
        return JsonToString(*left, {"name"}, "") < JsonToString(*right, {"name"}, "");
    });

    return agents;
}

static std::vector<const json*> CollectAbnormalities(const json& data) {
    std::vector<const json*> abnormalities;
    AppendArrayRefs(data, "abnormalities", abnormalities);

    std::sort(abnormalities.begin(), abnormalities.end(), [](const json* left, const json* right) {
        std::string leftName = JsonToString(*left, {"name", "code"}, "");
        std::string rightName = JsonToString(*right, {"name", "code"}, "");
        return leftName < rightName;
    });

    return abnormalities;
}

static std::string FormatDepartment(const json& agent) {
    std::string raw = JsonToString(agent, {"currentSefira", "_currentSefira", "currentSefiraEnum", "sefira", "department"}, "-");
    if (!HasText(raw)) return "-";

    std::string upper = ToUpper(raw);
    if (upper == "0" || upper == "NONE") return "None";
    if (upper == "1" || upper.find("MALKUTH") != std::string::npos || upper.find("CONTROL") != std::string::npos) return "Control Team";
    if (upper == "2" || upper.find("YESOD") != std::string::npos || upper.find("INFORMATION") != std::string::npos) return "Information Team";
    if (upper == "3" || upper.find("HOD") != std::string::npos || upper.find("TRAINING") != std::string::npos) return "Training Team";
    if (upper == "4" || upper.find("NETZACH") != std::string::npos || upper.find("SAFETY") != std::string::npos || upper.find("SECURITY") != std::string::npos) return "Safety Team";
    if (upper == "5" || upper == "6" || upper.find("TIPHERETH") != std::string::npos || upper.find("CENTRAL") != std::string::npos) return "Central Command";
    if (upper == "7" || upper.find("GEBURA") != std::string::npos || upper.find("DISCIPLINARY") != std::string::npos) return "Disciplinary Team";
    if (upper == "8" || upper.find("CHESED") != std::string::npos || upper.find("WELFARE") != std::string::npos) return "Welfare Team";
    if (upper == "9" || upper.find("HOKMA") != std::string::npos || upper.find("RECORD") != std::string::npos) return "Record Team";
    if (upper == "10" || upper.find("BINAH") != std::string::npos || upper.find("EXTRACTION") != std::string::npos) return "Extraction Team";
    if (upper == "11" || upper.find("KETER") != std::string::npos || upper.find("ARCHITECTURE") != std::string::npos) return "Architecture Team";

    try {
        int index = std::stoi(raw);
        static const char* departments[] = {
            "Control Team", "Information Team", "Training Team", "Safety Team",
            "Central Command", "Central Command", "Disciplinary Team", "Welfare Team", "Record Team", "Extraction Team", "Architecture Team", "None"
        };
        if (index >= 1 && index <= 12) return departments[index - 1];
        if (index == 0) return "None";
    } catch (...) {}

    return raw;
}

static int StatTier(int value) {
    if (value >= 100) return 6;
    if (value >= 85) return 5;
    if (value >= 65) return 4;
    if (value >= 45) return 3;
    if (value >= 30) return 2;
    return 1;
}

static const char* TierLabel(int tier) {
    switch (tier) {
        case 6: return "EX";
        case 5: return "V";
        case 4: return "IV";
        case 3: return "III";
        case 2: return "II";
        default: return "I";
    }
}

static int OverallLevelFromStatTiers(int totalTierLevels) {
    if (totalTierLevels >= 16) return 5;
    if (totalTierLevels >= 12) return 4;
    if (totalTierLevels >= 9) return 3;
    if (totalTierLevels >= 6) return 2;
    return 1;
}

static int NextOverallThreshold(int totalTierLevels) {
    if (totalTierLevels < 6) return 6;
    if (totalTierLevels < 9) return 9;
    if (totalTierLevels < 12) return 12;
    if (totalTierLevels < 16) return 16;
    return 0;
}

static EquipmentStats EmptyStats() {
    return EquipmentStats{0, 0, 0, 0};
}

static EquipmentStats AddStats(const EquipmentStats& a, const EquipmentStats& b) {
    return EquipmentStats{a.hp + b.hp, a.sp + b.sp, a.ws + b.ws, a.as + b.as};
}

static std::string EquipmentName(DatabaseManager& db, const json& item) {
    int typeId = JsonToInt(item, {"typeId", "equipmentId", "id"}, -1);
    if (typeId >= 0) {
        if (Equipment* equipment = db.GetEquipment(typeId)) return equipment->name;
    }

    std::string name = JsonToString(item, {"name", "displayName", "code"}, "");
    if (HasText(name)) return name;
    return typeId >= 0 ? ("Type #" + std::to_string(typeId)) : "Unknown item";
}

static std::string EquipmentType(DatabaseManager& db, const json& item) {
    int typeId = JsonToInt(item, {"typeId", "equipmentId", "id"}, -1);
    if (typeId >= 0) {
        if (Equipment* equipment = db.GetEquipment(typeId)) return equipment->type;
    }

    std::string type = JsonToString(item, {"type", "kind", "attachType"}, "-");
    return type;
}

static EquipmentStats EquipmentBonus(DatabaseManager& db, const json& item) {
    int typeId = JsonToInt(item, {"typeId", "equipmentId", "id"}, -1);
    if (typeId >= 0) {
        if (Equipment* equipment = db.GetEquipment(typeId)) return equipment->stats;
    }
    return EmptyStats();
}

static EquipmentStats GiftBonus(DatabaseManager& db, const json& agent) {
    EquipmentStats total = EmptyStats();
    const json* equipment = FindAny(agent, {"equipment"});
    if (equipment == nullptr || !equipment->is_object()) return total;

    const json* gifts = FindAny(*equipment, {"gifts", "giftList"});
    if (gifts == nullptr || !gifts->is_array()) return total;

    for (const auto& gift : *gifts) {
        if (!gift.is_object()) continue;
        total = AddStats(total, EquipmentBonus(db, gift));
    }

    return total;
}

static int AgentGiftCount(const json& agent) {
    const json* equipment = FindAny(agent, {"equipment"});
    if (equipment == nullptr || !equipment->is_object()) return 0;

    int explicitCount = JsonToInt(*equipment, {"giftCount"}, -1);
    if (explicitCount >= 0) return explicitCount;

    const json* gifts = FindAny(*equipment, {"gifts", "giftList"});
    return gifts != nullptr && gifts->is_array() ? static_cast<int>(gifts->size()) : 0;
}

static void DrawStatValue(int base, int bonus) {
    int total = base + bonus;
    if (bonus > 0) ImGui::Text("%d (%d +%d)", total, base, bonus);
    else if (bonus < 0) ImGui::Text("%d (%d %d)", total, base, bonus);
    else ImGui::Text("%d", base);
}

static std::string WorkStatusText(const json& agent) {
    const json* work = FindAny(agent, {"currentWork", "work", "currentSkill", "skill", "task"});
    if (work != nullptr) {
        if (work->is_string() || work->is_number() || work->is_boolean()) return JsonToString(work, "-");
        if (work->is_object()) {
            std::string category = JsonToString(*work, {"category", "workType", "type", "name"}, "");
            std::string target = JsonToString(*work, {"abnormalityName", "creatureName", "targetName", "target"}, "");
            std::string state = JsonToString(*work, {"state", "status"}, "");

            std::string text;
            if (HasText(category)) text += category;
            if (HasText(target)) text += text.empty() ? target : (" -> " + target);
            if (HasText(state)) text += text.empty() ? state : (" (" + state + ")");
            if (!text.empty()) return text;
        }
    }
    std::string deployment = JsonToString(agent, {"deployment"}, "");
    if (deployment == "spare") return "Reserve";
    return "Idle / no work field";
}

static std::string WorkTimeText(const json& source) {
    const json* work = FindAny(source, {"currentWork", "work", "currentSkill", "skill", "task"});
    const json* object = (work != nullptr && work->is_object()) ? work : &source;

    double remain = JsonToDouble(*object, {"remainTime", "remainingTime", "timeLeft", "left"}, -1.0);
    double total = JsonToDouble(*object, {"totalTime", "workTime", "duration", "maxTime"}, -1.0);
    double elapsed = JsonToDouble(*object, {"elapsedTime", "progressTime", "elapsed"}, -1.0);

    std::ostringstream out;
    if (remain >= 0.0) {
        out << std::fixed << std::setprecision(1) << remain << "s left";
        if (total >= 0.0) out << " / " << std::fixed << std::setprecision(1) << total << "s";
        return out.str();
    }
    if (elapsed >= 0.0 && total >= 0.0) {
        out << std::fixed << std::setprecision(1) << elapsed << "s / " << total << "s";
        return out.str();
    }
    if (total >= 0.0) {
        out << std::fixed << std::setprecision(1) << total << "s";
        return out.str();
    }
    return "-";
}

static int RiskPenalty(const std::string& risk) {
    std::string upper = ToUpper(risk);
    if (upper.find("ALEPH") != std::string::npos) return 45;
    if (upper.find("WAW") != std::string::npos) return 32;
    if (upper.find("HE") != std::string::npos) return 20;
    if (upper.find("TETH") != std::string::npos) return 10;
    return 0;
}

static std::string AbnormalityName(DatabaseManager& db, const json& abnormality) {
    int metadataId = JsonToInt(abnormality, {"metadataId", "metaId", "id"}, -1);
    if (metadataId >= 0) {
        if (Abnormality* dbAbno = db.GetAbnormality(metadataId)) return dbAbno->name;
    }

    std::string code = JsonToString(abnormality, {"code"}, "");
    if (HasText(code)) {
        if (Abnormality* dbAbno = db.GetAbnormalityByCode(code)) return dbAbno->name;
    }

    return JsonToString(abnormality, {"name", "code"}, "Unknown abnormality");
}

static std::string AbnormalityRisk(DatabaseManager& db, const json& abnormality) {
    int metadataId = JsonToInt(abnormality, {"metadataId", "metaId", "id"}, -1);
    if (metadataId >= 0) {
        if (Abnormality* dbAbno = db.GetAbnormality(metadataId)) return dbAbno->riskLevel;
    }

    std::string code = JsonToString(abnormality, {"code"}, "");
    if (HasText(code)) {
        if (Abnormality* dbAbno = db.GetAbnormalityByCode(code)) return dbAbno->riskLevel;
    }

    return JsonToString(abnormality, {"risk", "riskLevel", "grade"}, "-");
}

static int ObservationLevel(const json& abnormality) {
    return JsonToInt(abnormality, {"observationLevel", "observeLevel", "observe", "obLevel", "researchLevel"}, -1);
}

static bool ObservationComplete(const json& abnormality) {
    if (JsonToBool(abnormality, {"observationComplete", "isFullyObserved", "fullyObserved", "completed"}, false)) return true;

    int level = ObservationLevel(abnormality);
    if (level >= 4) return true;

    double percent = JsonToDouble(abnormality, {"observationPercent", "researchPercent", "completion"}, -1.0);
    return percent >= 100.0;
}

static double CurrentEnergy(const json& data) {
    const json* energy = FindAny(data, {"energy", "currentEnergy", "energyCurrent"});
    if (energy != nullptr && energy->is_object()) return JsonToDouble(*energy, {"current", "value", "amount", "energy"}, 0.0);
    return JsonToDouble(energy, 0.0);
}

static double EnergyGoal(const json& data) {
    const json* direct = FindAny(data, {"energyGoal", "requiredEnergy", "targetEnergy", "energyQuota", "maxEnergy"});
    if (direct != nullptr) return JsonToDouble(direct, -1.0);

    const json* energy = FindAny(data, {"energy", "energyInfo", "energyModel"});
    if (energy != nullptr && energy->is_object()) {
        return JsonToDouble(*energy, {"goal", "required", "target", "quota", "max"}, -1.0);
    }

    return -1.0;
}


static bool IsMainMenu(const json& liveData) {
    int day = JsonToInt(liveData, {"day"}, -1);
    int lob = JsonToInt(liveData, {"lob", "lobPoints"}, -1);
    const json* agents = FindAny(liveData, {"agents"});
    const json* abnormalities = FindAny(liveData, {"abnormalities"});
    bool noAgents = !agents || (agents->is_array() && agents->empty());
    bool noAbnos = !abnormalities || (abnormalities->is_array() && abnormalities->empty());
    return (day == 1 && lob == 0 && noAgents && noAbnos);
}

static void DrawMainMenuWarning() {
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::Yellow);
    ImGui::TextWrapped("You are in the main menu or the facility is empty.");
    ImGui::TextWrapped("Please load a save or start the game to view data.");
    ImGui::PopStyleColor();
}

void DrawAgentsTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError, DatabaseManager& db) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
        return;
    }
    if (IsMainMenu(liveData)) {
        DrawMainMenuWarning();
        return;
    }

    std::vector<const json*> allAgents = CollectAgents(liveData);
    std::vector<const json*> assignedAgents;
    std::vector<const json*> spareAgents;
    for (const json* agent : allAgents) {
        if (JsonToString(*agent, {"deployment"}, "") == "spare")
            spareAgents.push_back(agent);
        else
            assignedAgents.push_back(agent);
    }

    ImGui::BeginChild("##agents_tab", ImVec2(0, 0), false);
    SectionHeader("AGENTS");
    DrawMetricPill("Total", std::to_string(static_cast<int>(allAgents.size())));
    ImGui::SameLine();
    DrawMetricPill("Assigned", std::to_string(static_cast<int>(assignedAgents.size())), Colors::Green);
    ImGui::SameLine();
    DrawMetricPill("Reserve", std::to_string(static_cast<int>(spareAgents.size())), Colors::Yellow);
    ImGui::Spacing();

    auto drawAgentTable = [&](const char* tableId, std::vector<const json*>& agents, bool isAssigned) {
        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable;
        float tableHeight = isAssigned ? ImGui::GetContentRegionAvail().y * 0.6f : 0;
        if (ImGui::BeginTable(tableId, 9, flags, ImVec2(0, tableHeight))) {
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 130.0f);
            ImGui::TableSetupColumn("Unit", ImGuiTableColumnFlags_WidthStretch, 140.0f);
            ImGui::TableSetupColumn("Current work", ImGuiTableColumnFlags_WidthStretch, 190.0f);
            ImGui::TableSetupColumn("Overall", ImGuiTableColumnFlags_WidthStretch, 76.0f);
            ImGui::TableSetupColumn("Fortitude", ImGuiTableColumnFlags_WidthStretch, 88.0f);
            ImGui::TableSetupColumn("Prudence", ImGuiTableColumnFlags_WidthStretch, 88.0f);
            ImGui::TableSetupColumn("Temperance", ImGuiTableColumnFlags_WidthStretch, 100.0f);
            ImGui::TableSetupColumn("Justice", ImGuiTableColumnFlags_WidthStretch, 80.0f);
            ImGui::TableSetupColumn("Gifts", ImGuiTableColumnFlags_WidthStretch, 58.0f);
            ImGui::TableHeadersRow();

            if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
                if (sorts_specs->SpecsDirty || true) {
                    std::sort(agents.begin(), agents.end(), [sorts_specs](const json* a, const json* b) {
                        const ImGuiTableColumnSortSpecs* sort_spec = sorts_specs->Specs;
                        int delta = 0;
                        switch (sort_spec->ColumnIndex) {
                            case 0: {
                                std::string nameA = NormalizeText(JsonToString(*a, {"name"}, "Unnamed"));
                                std::string nameB = NormalizeText(JsonToString(*b, {"name"}, "Unnamed"));
                                delta = nameA.compare(nameB);
                                break;
                            }
                            case 1: {
                                std::string deptA = NormalizeText(FormatDepartment(*a));
                                std::string deptB = NormalizeText(FormatDepartment(*b));
                                delta = deptA.compare(deptB);
                                break;
                            }
                            case 3: {
                                int oa = OverallLevelFromStatTiers(std::min(StatTier(JsonToInt(*a, {"fortitude"}, 0)), 5) + std::min(StatTier(JsonToInt(*a, {"prudence"}, 0)), 5) + std::min(StatTier(JsonToInt(*a, {"temperance"}, 0)), 5) + std::min(StatTier(JsonToInt(*a, {"justice"}, 0)), 5));
                                int ob = OverallLevelFromStatTiers(std::min(StatTier(JsonToInt(*b, {"fortitude"}, 0)), 5) + std::min(StatTier(JsonToInt(*b, {"prudence"}, 0)), 5) + std::min(StatTier(JsonToInt(*b, {"temperance"}, 0)), 5) + std::min(StatTier(JsonToInt(*b, {"justice"}, 0)), 5));
                                delta = oa - ob;
                                break;
                            }
                            case 4: delta = JsonToInt(*a, {"fortitude"}, 0) - JsonToInt(*b, {"fortitude"}, 0); break;
                            case 5: delta = JsonToInt(*a, {"prudence"}, 0) - JsonToInt(*b, {"prudence"}, 0); break;
                            case 6: delta = JsonToInt(*a, {"temperance"}, 0) - JsonToInt(*b, {"temperance"}, 0); break;
                            case 7: delta = JsonToInt(*a, {"justice"}, 0) - JsonToInt(*b, {"justice"}, 0); break;
                            case 8: delta = AgentGiftCount(*a) - AgentGiftCount(*b); break;
                            default: break;
                        }
                        if (delta > 0) return sort_spec->SortDirection == ImGuiSortDirection_Ascending ? false : true;
                        if (delta < 0) return sort_spec->SortDirection == ImGuiSortDirection_Ascending ? true : false;
                        return false;
                    });
                    sorts_specs->SpecsDirty = false;
                }
            }

            for (const json* agent : agents) {
                EquipmentStats giftBonus = GiftBonus(db, *agent);
                int fortitude = JsonToInt(*agent, {"fortitude"}, 0);
                int prudence = JsonToInt(*agent, {"prudence"}, 0);
                int temperance = JsonToInt(*agent, {"temperance"}, 0);
                int justice = JsonToInt(*agent, {"justice"}, 0);
                int totalTier = std::min(StatTier(fortitude), 5) + std::min(StatTier(prudence), 5) +
                                std::min(StatTier(temperance), 5) + std::min(StatTier(justice), 5);
                int calculatedLevel = OverallLevelFromStatTiers(totalTier);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", JsonToString(*agent, {"name"}, "Unnamed").c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", FormatDepartment(*agent).c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", WorkStatusText(*agent).c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", TierLabel(calculatedLevel));
                ImGui::SameLine();
                ImGui::TextColored(Colors::Muted, "(%d)", totalTier);
                ImGui::TableSetColumnIndex(4);
                DrawStatValue(fortitude, giftBonus.hp);
                ImGui::TableSetColumnIndex(5);
                DrawStatValue(prudence, giftBonus.sp);
                ImGui::TableSetColumnIndex(6);
                DrawStatValue(temperance, giftBonus.ws);
                ImGui::TableSetColumnIndex(7);
                DrawStatValue(justice, giftBonus.as);
                ImGui::TableSetColumnIndex(8);
                ImGui::Text("%d", AgentGiftCount(*agent));
            }
            ImGui::EndTable();
        }
    };

    if (ImGui::CollapsingHeader("Assigned Agents", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (assignedAgents.empty()) {
            ImGui::TextColored(Colors::Muted, "No agents currently assigned to departments.");
        } else {
            drawAgentTable("##assigned_agents_table", assignedAgents, true);
        }
        ImGui::Spacing();
    }

    if (ImGui::CollapsingHeader("Reserve Agents", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (spareAgents.empty()) {
            ImGui::TextColored(Colors::Muted, "No reserve agents available.");
        } else {
            drawAgentTable("##reserve_agents_table", spareAgents, false);
        }
        ImGui::Spacing();
    }

    ImGui::EndChild();
}

struct InventoryRow {
    int typeId = -1;
    std::string name;
    std::string type;
    int total = 0;
    int assigned = 0;
    EquipmentStats stats = {0, 0, 0, 0};
    std::set<std::string> owners;
};

static std::string OwnerList(const std::set<std::string>& owners) {
    if (owners.empty()) return "-";
    std::string out;
    for (const std::string& owner : owners) {
        if (!out.empty()) out += ", ";
        out += owner;
    }
    return out;
}

static void AddInventoryItem(std::map<std::string, InventoryRow>& rows,
                             std::set<std::string>& countedInstances,
                             std::set<std::string>& assignedInstances,
                             DatabaseManager& db,
                             const json& item,
                             const std::string& fallbackOwner) {
    if (!item.is_object()) return;

    std::string name = EquipmentName(db, item);
    std::string type = EquipmentType(db, item);
    int typeId = JsonToInt(item, {"typeId", "equipmentId", "id"}, -1);
    std::string rowKey = name + "_" + type;
    std::string instanceId = JsonToString(item, {"instanceId"}, "");
    std::string instanceKey = HasText(instanceId) ? ("inst:" + instanceId) : "";

    InventoryRow& row = rows[rowKey];
    row.typeId = typeId;
    row.name = name;
    row.type = type;
    row.stats = EquipmentBonus(db, item);

    bool counted = false;
    if (instanceKey.empty()) {
        row.total++;
        counted = true;
    } else if (!countedInstances.count(instanceKey)) {
        countedInstances.insert(instanceKey);
        row.total++;
        counted = true;
    }

    std::string owner = JsonToString(item, {"ownerName", "owner"}, "");
    if (!HasText(owner)) owner = fallbackOwner;
    bool assigned = HasText(owner);

    if (assigned) {
        row.owners.insert(owner);
        if (instanceKey.empty()) {
            row.assigned++;
        } else if (!assignedInstances.count(instanceKey)) {
            assignedInstances.insert(instanceKey);
            row.assigned++;
        } else if (!counted) {
            row.assigned = std::min(row.assigned, row.total);
        }
    }
}

static void AddAgentEquipmentToInventory(std::map<std::string, InventoryRow>& rows,
                                         std::set<std::string>& countedInstances,
                                         std::set<std::string>& assignedInstances,
                                         DatabaseManager& db,
                                         const json& agent) {
    std::string owner = JsonToString(agent, {"name"}, "");
    const json* equipment = FindAny(agent, {"equipment"});
    if (equipment == nullptr || !equipment->is_object()) return;

    const json* weapon = FindAny(*equipment, {"weapon"});
    if (weapon != nullptr && weapon->is_object()) AddInventoryItem(rows, countedInstances, assignedInstances, db, *weapon, owner);

    const json* armor = FindAny(*equipment, {"armor"});
    if (armor != nullptr && armor->is_object()) AddInventoryItem(rows, countedInstances, assignedInstances, db, *armor, owner);

    const json* gifts = FindAny(*equipment, {"gifts", "giftList"});
    if (gifts != nullptr && gifts->is_array()) {
        for (const auto& gift : *gifts) {
            AddInventoryItem(rows, countedInstances, assignedInstances, db, gift, owner);
        }
    }
}

static std::vector<InventoryRow> BuildInventoryRows(const json& liveData, DatabaseManager& db) {
    std::map<std::string, InventoryRow> rows;
    std::set<std::string> countedInstances;
    std::set<std::string> assignedInstances;

    const json* inventory = FindAny(liveData, {"inventory"});
    if (inventory != nullptr && inventory->is_object()) {
        const json* items = FindAny(*inventory, {"items"});
        if (items != nullptr && items->is_array()) {
            for (const auto& item : *items) {
                AddInventoryItem(rows, countedInstances, assignedInstances, db, item, "");
            }
        }
    }

    for (const json* agent : CollectAgents(liveData)) {
        AddAgentEquipmentToInventory(rows, countedInstances, assignedInstances, db, *agent);
    }

    std::vector<InventoryRow> result;
    for (auto& pair : rows) {
        pair.second.assigned = std::min(pair.second.assigned, pair.second.total);
        result.push_back(pair.second);
    }
    return result;
}

void DrawInventoryTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError, DatabaseManager& db) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
        return;
    }
    if (IsMainMenu(liveData)) {
        DrawMainMenuWarning();
        return;
    }

    std::vector<InventoryRow> rows = BuildInventoryRows(liveData, db);
    std::vector<InventoryRow> equipmentRows;
    std::vector<InventoryRow> giftRows;

    for (auto& row : rows) {
        if (row.type == "GIFT") giftRows.push_back(row);
        else equipmentRows.push_back(row);
    }

    int total = 0;
    int assigned = 0;
    for (const InventoryRow& row : rows) {
        total += row.total;
        assigned += row.assigned;
    }

    ImGui::BeginChild("##inventory_tab", ImVec2(0, 0), false);
    SectionHeader("INVENTORY");
    DrawMetricPill("Types", std::to_string(static_cast<int>(rows.size())));
    ImGui::SameLine();
    DrawMetricPill("Total", std::to_string(total));
    ImGui::SameLine();
    DrawMetricPill("Assigned", std::to_string(assigned), Colors::Yellow);
    ImGui::SameLine();
    DrawMetricPill("Free", std::to_string(total - assigned), Colors::Green);
    ImGui::Spacing();

    static char searchFilterEquip[128] = "";
    static char searchFilterGift[128] = "";

    auto sortFunc = [](const InventoryRow& left, const InventoryRow& right, const ImGuiTableSortSpecs* sorts_specs) {
        for (int n = 0; n < sorts_specs->SpecsCount; n++) {
            const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[n];
            int delta = 0;
            switch (sort_spec->ColumnIndex) {
                case 0: delta = left.name.compare(right.name); break;
                case 1: delta = left.type.compare(right.type); break;
                case 2: delta = left.total - right.total; break;
                case 3: delta = left.assigned - right.assigned; break;
                case 4: delta = (left.total - left.assigned) - (right.total - right.assigned); break;
                default: break;
            }
            if (delta > 0) return sort_spec->SortDirection == ImGuiSortDirection_Ascending;
            if (delta < 0) return sort_spec->SortDirection == ImGuiSortDirection_Descending;
        }
        return left.name < right.name;
    };

    auto renderTable = [&](const char* id, std::vector<InventoryRow>& tableRows, bool isGift, const std::string& searchStr) {
        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable;
        int numCols = isGift ? 3 : 6;
        if (ImGui::BeginTable(id, numCols, flags, ImVec2(0, 300))) {
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 220.0f);
            if (!isGift) {
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 80.0f);
                ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthStretch, 58.0f);
                ImGui::TableSetupColumn("Assigned", ImGuiTableColumnFlags_WidthStretch, 74.0f);
                ImGui::TableSetupColumn("Free", ImGuiTableColumnFlags_WidthStretch, 58.0f);
            } else {
                ImGui::TableSetupColumn("Gift stats", ImGuiTableColumnFlags_WidthStretch, 155.0f);
            }
            ImGui::TableSetupColumn("Assigned to", ImGuiTableColumnFlags_WidthStretch, 240.0f);
            ImGui::TableHeadersRow();

            if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
                if (sorts_specs->SpecsDirty || true) {
                    std::sort(tableRows.begin(), tableRows.end(), [&](const InventoryRow& a, const InventoryRow& b) {
                        return sortFunc(a, b, sorts_specs);
                    });
                    sorts_specs->SpecsDirty = false;
                }
            }

            for (const InventoryRow& row : tableRows) {
                if (searchStr.length() > 0) {
                    std::string rowNameStr = row.name;
                    std::transform(rowNameStr.begin(), rowNameStr.end(), rowNameStr.begin(), ::tolower);
                    if (rowNameStr.find(searchStr) == std::string::npos) continue;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", row.name.c_str());
                if (ImGui::IsItemHovered()) {
                    Equipment* eq = db.GetEquipment(row.typeId);
                    if (eq) {
                        if (row.type == "WEAPON") {
                            ImGui::SetTooltip("Abnormality: %s\nWeapon Type: %s\nDamage: %s (%s)\nSpeed: %s\nRange: %s",
                                eq->abnormalityName.empty() ? "-" : eq->abnormalityName.c_str(),
                                eq->weaponType.empty() ? "-" : eq->weaponType.c_str(),
                                eq->dmg.empty() ? "-" : eq->dmg.c_str(),
                                eq->dmgType.empty() ? "-" : eq->dmgType.c_str(),
                                eq->attackSpeed.empty() ? "-" : eq->attackSpeed.c_str(),
                                eq->range.empty() ? "-" : eq->range.c_str());
                        } else if (row.type == "ARMOR") {
                            ImGui::SetTooltip("Abnormality: %s\nRED Def: %s\nWHITE Def: %s\nBLACK Def: %s\nPALE Def: %s",
                                eq->abnormalityName.empty() ? "-" : eq->abnormalityName.c_str(),
                                eq->defenseRed.empty() ? "-" : eq->defenseRed.c_str(),
                                eq->defenseWhite.empty() ? "-" : eq->defenseWhite.c_str(),
                                eq->defenseBlack.empty() ? "-" : eq->defenseBlack.c_str(),
                                eq->defensePale.empty() ? "-" : eq->defensePale.c_str());
                        } else if (row.type == "GIFT") {
                            ImGui::SetTooltip("Abnormality: %s\nSlot: %s\nHP: %+d\nSP: %+d\nWork Speed: %+d\nAttack Speed: %+d",
                                eq->abnormalityName.empty() ? "-" : eq->abnormalityName.c_str(),
                                eq->attachmentSlot.empty() ? "-" : eq->attachmentSlot.c_str(),
                                eq->stats.hp, eq->stats.sp, eq->stats.ws, eq->stats.as);
                        } else {
                            if (row.stats.hp != 0 || row.stats.sp != 0 || row.stats.ws != 0 || row.stats.as != 0) {
                                ImGui::SetTooltip("Stat Bonuses:\nHP: %+d\nSP: %+d\nWork Speed: %+d\nAttack Speed: %+d",
                                    row.stats.hp, row.stats.sp, row.stats.ws, row.stats.as);
                            }
                        }
                    } else {
                        if (row.stats.hp != 0 || row.stats.sp != 0 || row.stats.ws != 0 || row.stats.as != 0) {
                            ImGui::SetTooltip("Stat Bonuses:\nHP: %+d\nSP: %+d\nWork Speed: %+d\nAttack Speed: %+d",
                                row.stats.hp, row.stats.sp, row.stats.ws, row.stats.as);
                        } else if (row.type == "WEAPON" || row.type == "ARMOR") {
                            ImGui::SetTooltip("Advanced stats for %s are not extracted in this version.", row.type.c_str());
                        }
                    }
                }

                if (!isGift) {
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(Colors::Yellow, "%s", row.type.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%d", row.total);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%d", row.assigned);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%d", row.total - row.assigned);
                    ImGui::TableSetColumnIndex(5);
                } else {
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(Colors::Green, "HP:%d SP:%d WS:%d AS:%d", row.stats.hp, row.stats.sp, row.stats.ws, row.stats.as);
                    ImGui::TableSetColumnIndex(2);
                }

                if (row.owners.empty()) {
                    ImGui::TextColored(Colors::Muted, "-");
                } else {
                    ImGui::PushID(row.typeId);
                    if (ImGui::TreeNode((void*)(intptr_t)(row.typeId + 1), "Assigned list (%d)", (int)row.owners.size())) {
                        ImGui::TextWrapped("%s", OwnerList(row.owners).c_str());
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
    };

    if (ImGui::CollapsingHeader("WEAPONS & ARMORS", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Search Equipment", searchFilterEquip, 128);
        ImGui::Spacing();
        std::string searchStrEquip = searchFilterEquip;
        std::transform(searchStrEquip.begin(), searchStrEquip.end(), searchStrEquip.begin(), ::tolower);
        renderTable("##inventory_table_equip", equipmentRows, false, searchStrEquip);
        ImGui::Spacing();
    }
    if (ImGui::CollapsingHeader("E.G.O GIFTS", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("Search Gifts", searchFilterGift, 128);
        ImGui::Spacing();
        std::string searchStrGift = searchFilterGift;
        std::transform(searchStrGift.begin(), searchStrGift.end(), searchStrGift.begin(), ::tolower);
        renderTable("##inventory_table_gift", giftRows, true, searchStrGift);
        ImGui::Spacing();
    }
    ImGui::EndChild();
}

static std::vector<const json*> CollectOrdealRows(const json& ordeal) {
    std::vector<const json*> rows;
    for (const char* key : {"today", "queue", "ordeals", "items", "upcoming", "schedule"}) {
        const json* array = FindAny(ordeal, {key});
        if (array != nullptr && array->is_array()) {
            for (const auto& item : *array) {
                if (item.is_object()) rows.push_back(&item);
            }
        }
        if (!rows.empty()) break;
    }
    return rows;
}

void DrawOrdealTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
        return;
    }
    if (IsMainMenu(liveData)) {
        DrawMainMenuWarning();
        return;
    }

    ImGui::BeginChild("##ordeal_tab", ImVec2(0, 0), false);
    SectionHeader("TODAY'S ORDEAL");
    const json* ordeal = FindAny(liveData, {"ordeal"});
    if (ordeal == nullptr || !ordeal->is_object()) {
        ImGui::TextColored(Colors::Muted, "No ordeal data in live snapshot yet.");
        ImGui::EndChild();
        return;
    }

    std::vector<const json*> rows = CollectOrdealRows(*ordeal);
    if (rows.empty()) {
        ImGui::TextColored(Colors::Green, "No Ordeals detected.");
        ImGui::EndChild();
        return;
    }

    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##ordeal_table", 5, flags, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthStretch, 42.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Level");
        ImGui::TableSetupColumn("Color / Type");
        ImGui::TableSetupColumn("Trigger");
        ImGui::TableHeadersRow();

        for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
            const json& row = *rows[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", i + 1);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", JsonToString(row, {"name", "ordealName", "displayName"}, "-").c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", JsonToString(row, {"level", "ordealLevel"}, "-").c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", JsonToString(row, {"color", "type", "kind"}, "-").c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", JsonToString(row, {"trigger", "time", "meltdown"}, "-").c_str());
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

static void DrawLevelRulesTable() {
    if (ImGui::BeginTable("##level_rules", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Base Level");
        ImGui::TableSetupColumn("I");
        ImGui::TableSetupColumn("II");
        ImGui::TableSetupColumn("III");
        ImGui::TableSetupColumn("IV");
        ImGui::TableSetupColumn("V");
        ImGui::TableSetupColumn("EX");
        ImGui::TableHeadersRow();

        const char* rows[][7] = {
            {"Values (Minimum)", "<30", "30", "45", "65", "85", "100+"},
            {"LOB Points Cost", "X", "1", "2", "3", "4", "6"},
            {"Justice Upgrade", "X", "3", "6", "9", "12", "18"},
            {"Total Stat Levels", "4", "6 (2)", "9 (3)", "12 (4)", "16 (5)", "X"}
        };

        for (const auto& row : rows) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", row[0]);
            for (int column = 1; column < 7; ++column) {
                ImGui::TableSetColumnIndex(column);
                ImGui::Text("%s", row[column]);
            }
        }
        ImGui::EndTable();
    }
}

static double StatLevelProgress(int value) {
    if (value >= 100) return 100.0;
    if (value >= 85) return (value - 85) / 15.0 * 100.0;
    if (value >= 65) return (value - 65) / 20.0 * 100.0;
    if (value >= 45) return (value - 45) / 20.0 * 100.0;
    if (value >= 30) return (value - 30) / 15.0 * 100.0;
    return value / 30.0 * 100.0;
}

void DrawExpTrackerTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
        return;
    }
    if (IsMainMenu(liveData)) {
        DrawMainMenuWarning();
        return;
    }

    std::vector<const json*> agents = CollectAgents(liveData, true); // working only
    if (agents.empty()) {
        ImGui::TextColored(Colors::Muted, "No active agents to track.");
        return;
    }

    ImGui::BeginChild("##exp_tracker", ImVec2(0, 0), false);
    SectionHeader("EXP TRACKER");
    ImGui::TextColored(Colors::Muted, "Missing EXP for next level (based on mod / base game thresholds).");

    static bool showOnlyAlmostLeveled = false;
    ImGui::Checkbox("Show only agents close to leveling up", &showOnlyAlmostLeveled);
    ImGui::Spacing();

    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##agent_exp_table", 5, flags, ImVec2(0, 300))) {
        ImGui::TableSetupColumn("Agent Name", ImGuiTableColumnFlags_WidthStretch, 150.0f);
        ImGui::TableSetupColumn("Fortitude (R)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Prudence (W)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Temperance (B)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Justice (P)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const json* agent : agents) {
            int statHp = JsonToInt(*agent, {"fortitude"}, 0) + static_cast<int>(JsonToDouble(*agent, {"expHp"}, 0.0));
            int lvlHp = StatTier(statHp);
            double progHp = StatLevelProgress(statHp);

            int statMental = JsonToInt(*agent, {"prudence"}, 0) + static_cast<int>(JsonToDouble(*agent, {"expMental"}, 0.0));
            int lvlMental = StatTier(statMental);
            double progMental = StatLevelProgress(statMental);

            int statWork = JsonToInt(*agent, {"temperance"}, 0) + static_cast<int>(JsonToDouble(*agent, {"expWork"}, 0.0));
            int lvlWork = StatTier(statWork);
            double progWork = StatLevelProgress(statWork);

            int statBattle = JsonToInt(*agent, {"justice"}, 0) + static_cast<int>(JsonToDouble(*agent, {"expBattle"}, 0.0));
            int lvlBattle = StatTier(statBattle);
            double progBattle = StatLevelProgress(statBattle);

            if (lvlHp == 6 && lvlMental == 6 && lvlWork == 6 && lvlBattle == 6) {
                continue; // Skip agents with all stats at EX
            }

            if (showOnlyAlmostLeveled) {
                bool almostLeveled = false;
                if (progHp >= 90.0 && lvlHp < 6) almostLeveled = true;
                if (progMental >= 90.0 && lvlMental < 6) almostLeveled = true;
                if (progWork >= 90.0 && lvlWork < 6) almostLeveled = true;
                if (progBattle >= 90.0 && lvlBattle < 6) almostLeveled = true;

                if (!almostLeveled) continue;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", JsonToString(*agent, {"name"}, "Unnamed").c_str());

            // Fortitude
            ImGui::TableSetColumnIndex(1);
            if (lvlHp < 6) ImGui::TextColored(Colors::Yellow, "%s (%.1f%%)", TierLabel(lvlHp), progHp);
            else ImGui::TextColored(Colors::Green, "EX");

            // Prudence
            ImGui::TableSetColumnIndex(2);
            if (lvlMental < 6) ImGui::TextColored(Colors::Yellow, "%s (%.1f%%)", TierLabel(lvlMental), progMental);
            else ImGui::TextColored(Colors::Green, "EX");

            // Temperance
            ImGui::TableSetColumnIndex(3);
            if (lvlWork < 6) ImGui::TextColored(Colors::Yellow, "%s (%.1f%%)", TierLabel(lvlWork), progWork);
            else ImGui::TextColored(Colors::Green, "EX");

            // Justice
            ImGui::TableSetColumnIndex(4);
            if (lvlBattle < 6) ImGui::TextColored(Colors::Yellow, "%s (%.1f%%)", TierLabel(lvlBattle), progBattle);
            else ImGui::TextColored(Colors::Green, "EX");
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

static double PredictSuccessRate(const json& agent, const json& abnormality, DatabaseManager& db, int workCategory) {
    const char* statKeys[] = {"fortitude", "prudence", "temperance", "justice"};
    int stat = JsonToInt(agent, {statKeys[workCategory]}, 0);
    int agentLevel = JsonToInt(agent, {"level", "grade"}, OverallLevelFromStatTiers(
        std::min(StatTier(JsonToInt(agent, {"fortitude"}, 0)), 5) +
        std::min(StatTier(JsonToInt(agent, {"prudence"}, 0)), 5) +
        std::min(StatTier(JsonToInt(agent, {"temperance"}, 0)), 5) +
        std::min(StatTier(JsonToInt(agent, {"justice"}, 0)), 5)));
    int observation = std::max(0, ObservationLevel(abnormality));
    std::string risk = AbnormalityRisk(db, abnormality);

    double rate = 50.0 + (static_cast<double>(stat) - 50.0) * 0.45 + agentLevel * 2.5 + observation * 3.0 - RiskPenalty(risk);
    if (rate < 5.0) rate = 5.0;
    if (rate > 95.0) rate = 95.0;
    return rate;
}

static void DrawPredictionPanel(const std::vector<const json*>& agents,
                                const std::vector<const json*>& abnormalities,
                                DatabaseManager& db) {
    static int selectedAgent = 0;
    static int selectedAbno = 0;
    static int selectedWork = 0;
    const char* workNames[] = {"Instinct", "Insight", "Attachment", "Repression"};

    if (agents.empty() || abnormalities.empty()) {
        ImGui::TextColored(Colors::Muted, "Prediction needs at least one agent and one abnormality.");
        return;
    }

    if (selectedAgent >= static_cast<int>(agents.size())) selectedAgent = 0;
    if (selectedAbno >= static_cast<int>(abnormalities.size())) selectedAbno = 0;

    ImGui::SetNextItemWidth(190.0f);
    if (ImGui::BeginCombo("Agent", JsonToString(*agents[selectedAgent], {"name"}, "Unnamed").c_str())) {
        for (int i = 0; i < static_cast<int>(agents.size()); ++i) {
            bool selected = i == selectedAgent;
            if (ImGui::Selectable(JsonToString(*agents[i], {"name"}, "Unnamed").c_str(), selected)) selectedAgent = i;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::BeginCombo("Abnormality", AbnormalityName(db, *abnormalities[selectedAbno]).c_str())) {
        for (int i = 0; i < static_cast<int>(abnormalities.size()); ++i) {
            bool selected = i == selectedAbno;
            if (ImGui::Selectable(AbnormalityName(db, *abnormalities[i]).c_str(), selected)) selectedAbno = i;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::BeginCombo("Work", workNames[selectedWork])) {
        for (int i = 0; i < 4; ++i) {
            bool selected = i == selectedWork;
            if (ImGui::Selectable(workNames[i], selected)) selectedWork = i;
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    double rate = PredictSuccessRate(*agents[selectedAgent], *abnormalities[selectedAbno], db, selectedWork);
    ImGui::Spacing();
    ImGui::TextColored(Colors::Accent, "Estimated success rate: %.0f%%", rate);
    ImGui::SameLine();
    ImGui::TextColored(Colors::Muted, "(C++ fallback estimate until exact game formula is exported)");
}

static std::vector<const json*> CollectWorkRows(const json& liveData) {
    std::vector<const json*> rows;
    for (const char* key : {"workHistory", "recentWorks", "workRecords", "works"}) {
        const json* array = FindAny(liveData, {key});
        if (array != nullptr && array->is_array()) {
            for (const auto& item : *array) {
                if (item.is_object()) rows.push_back(&item);
            }
        }
    }
    return rows;
}

static void DrawEnergyBreakdown(const json& liveData, const std::vector<const json*>& abnormalities) {
    double current = 0.0;
    double goal = -1.0;
    const json* energyDetails = FindAny(liveData, {"energyDetails"});
    if (energyDetails && energyDetails->is_object()) {
        current = JsonToDouble(*energyDetails, {"current"}, 0.0);
        goal = JsonToDouble(*energyDetails, {"need"}, -1.0);
    } else {
        current = CurrentEnergy(liveData);
        goal = EnergyGoal(liveData);
    }

    double missing = goal >= 0.0 ? std::max(0.0, goal - current) : -1.0;

    DrawMetric("Day", JsonToString(liveData, {"day"}, "-"));
    DrawMetric("Energy", std::to_string(static_cast<int>(current)));
    DrawMetric("Energy missing", missing >= 0.0 ? std::to_string(static_cast<int>(missing)) : "No quota field");
}

static void DrawAgentStateTable(const std::vector<const json*>& agents) {
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##agent_state_table", 7, flags, ImVec2(0, 180))) {
        ImGui::TableSetupColumn("Agent");
        ImGui::TableSetupColumn("Unit");
        ImGui::TableSetupColumn("HP");
        ImGui::TableSetupColumn("SP");
        ImGui::TableSetupColumn("Panic in SP");
        ImGui::TableSetupColumn("Current work");
        ImGui::TableSetupColumn("Work time");
        ImGui::TableHeadersRow();

        for (const json* agent : agents) {
            int hp = JsonToInt(*agent, {"hp"}, 0);
            int maxHp = JsonToInt(*agent, {"maxHp"}, 0);
            int sp = JsonToInt(*agent, {"sp"}, 0);
            int maxSp = JsonToInt(*agent, {"maxSp"}, 0);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", JsonToString(*agent, {"name"}, "Unnamed").c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", FormatDepartment(*agent).c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d/%d", hp, maxHp);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d/%d", sp, maxSp);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%d", sp);
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%s", WorkStatusText(*agent).c_str());
            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%s", WorkTimeText(*agent).c_str());
        }
        ImGui::EndTable();
    }
}

static void DrawObservationTable(const std::vector<const json*>& abnormalities, DatabaseManager& db) {
    int complete = 0;
    for (const json* abnormality : abnormalities) {
        if (ObservationComplete(*abnormality)) complete++;
    }

    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##observation_table", 6, flags, ImVec2(0, 190))) {
        ImGui::TableSetupColumn("Abnormality");
        ImGui::TableSetupColumn("Code");
        ImGui::TableSetupColumn("Risk");
        ImGui::TableSetupColumn("Qliphoth");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("PE Boxes");
        ImGui::TableHeadersRow();

        for (const json* abnormality : abnormalities) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            std::string name = AbnormalityName(db, *abnormality);
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Link);
            if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                std::string url = "";
                int metadataId = JsonToInt(*abnormality, {"metadataId", "metaId", "id"}, -1);
                if (metadataId >= 0) {
                    if (Abnormality* dbAbno = db.GetAbnormality(metadataId)) url = dbAbno->wikiLink;
                }
                if (url.empty()) {
                    std::string code = JsonToString(*abnormality, {"code"}, "");
                    if (HasText(code)) {
                        if (Abnormality* dbAbno = db.GetAbnormalityByCode(code)) url = dbAbno->wikiLink;
                    }
                }
                
                if (!url.empty()) {
                    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
            }
            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", JsonToString(*abnormality, {"code"}, "-").c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", AbnormalityRisk(db, *abnormality).c_str());
            ImGui::TableSetColumnIndex(3);
            std::string state = JsonToString(*abnormality, {"state"}, "IDLE");

            int qliphoth = JsonToInt(*abnormality, {"qliphothCounter"}, -1);
            if (qliphoth > 0) ImGui::Text("%d", qliphoth);
            else if (qliphoth == 0 && state == "ESCAPE") ImGui::TextColored(Colors::Red, "0");
            else ImGui::TextColored(Colors::Muted, "Non-Escapable");

            ImGui::TableSetColumnIndex(4);
            std::string stateStr = JsonToString(*abnormality, {"state"}, "IDLE");
            std::string worker = JsonToString(*abnormality, {"worker"}, "");
            bool isMeltdown = JsonToBool(*abnormality, {"isMeltdown"}, false);
            
            if (isMeltdown) {
                float timer = JsonToDouble(*abnormality, {"meltdownTimer"}, 60.0);
                ImGui::TextColored(Colors::Red, "MELTDOWN (%.0fs)", timer);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImColor(100, 0, 0, 150));
            } else if (stateStr == "ESCAPE") {
                ImGui::TextColored(Colors::Red, "BREACHING");
            } else if (stateStr == "WORKING" && !worker.empty()) {
                ImGui::TextColored(Colors::Yellow, "Working (%s)", worker.c_str());
            } else if (stateStr == "IDLE" || stateStr == "WAIT") {
                ImGui::TextColored(Colors::Green, "Idle");
            } else {
                ImGui::Text("%s", stateStr.c_str());
            }
            
            ImGui::TableSetColumnIndex(5);
            std::string codeForToolCheck = JsonToString(*abnormality, {"code"}, "");
            bool isTool = (codeForToolCheck.find("-09-") != std::string::npos);
            int peBoxes = JsonToInt(*abnormality, {"peBox"}, -1);
            
            if (isTool) {
                ImGui::TextColored(Colors::Muted, "Cannot extract PE");
            } else if (peBoxes >= 0) {
                ImGui::Text("%d", peBoxes);
            } else {
                ImGui::TextColored(Colors::Muted, "-");
            }
        }
        ImGui::EndTable();
    }
}

void DrawDetailedInfoTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError, DatabaseManager& db) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
        return;
    }
    if (IsMainMenu(liveData)) {
        DrawMainMenuWarning();
        return;
    }

    ImGui::BeginChild("##detailed_info_tab", ImVec2(0, 0), false);
    SectionHeader("DETAILED INFO");

    std::vector<const json*> agents = CollectAgents(liveData, true);
    std::vector<const json*> abnormalities = CollectAbnormalities(liveData);

    if (ImGui::CollapsingHeader("Facility and energy", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(2, "facility_energy_cols", false);
        DrawEnergyBreakdown(liveData, abnormalities);
        ImGui::NextColumn();
        // Here we can put additional high-level facility info if needed.
        int lobPoints = JsonToInt(liveData, {"lob", "lobPoints"}, 0);
        ImGui::Text("LOB Points: ");
        ImGui::SameLine();
        ImGui::TextColored(Colors::Yellow, "%d", lobPoints);
        ImGui::Columns(1);
        ImGui::Spacing();
    }

    // Agents and abnormalities vectors are already initialized above.



    if (ImGui::CollapsingHeader("Observation catalog", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawObservationTable(abnormalities, db);
        ImGui::Spacing();
    }

    if (ImGui::CollapsingHeader("Success rate prediction", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawPredictionPanel(agents, abnormalities, db);
        ImGui::Spacing();
    }

    ImGui::EndChild();
}

void DrawExtractionTab(const json& liveData, DatabaseManager& db) {
    if (IsMainMenu(liveData)) {
        DrawMainMenuWarning();
        return;
    }
    ImGui::BeginChild("##extraction_tab", ImVec2(0, 0), false);
    SectionHeader("NEW ABNORMALITY SELECTION");
    ImGui::Spacing();

    bool isExtracting = JsonToBool(liveData, {"isExtracting"}, false);
    const json* choices = FindAny(liveData, {"extractionChoices"});
    if (!isExtracting || !choices || !choices->is_array() || choices->empty()) {
        ImGui::TextColored(Colors::Yellow, "No active extraction event.");
        ImGui::EndChild();
        return;
    }

    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##extraction_table", 3, flags)) {
        ImGui::TableSetupColumn("Box");
        ImGui::TableSetupColumn("Code");
        ImGui::TableSetupColumn("Risk");
        ImGui::TableHeadersRow();

        int index = 1;
        for (auto it = choices->rbegin(); it != choices->rend(); ++it) {
            const auto& choice = *it;
            long creatureId = -1;
            try {
                if (choice.is_number()) creatureId = choice.get<long>();
                else if (choice.is_string()) creatureId = std::stol(choice.get<std::string>());
            } catch (...) {
                creatureId = -1;
            }

            if (creatureId < 0) continue;

            Abnormality* dbAbno = db.GetAbnormality(creatureId);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            std::string name = "Unknown";
            std::string code = "-";
            std::string risk = "-";

            if (dbAbno) {
                name = dbAbno->name;
                code = dbAbno->code;
                risk = dbAbno->riskLevel;
            }

            std::string label = "Box " + std::to_string(index) + ": " + name;
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Link);
            if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                if (dbAbno && !dbAbno->wikiLink.empty()) {
                    ShellExecuteA(NULL, "open", dbAbno->wikiLink.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
            }
            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", code.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", risk.c_str());

            index++;
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void DrawInfoTab() {
    ImGui::BeginChild("##info_tab", ImVec2(0, 0), false);

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::Accent);
    ImGui::SetWindowFontScale(1.2f);
    ImGui::Text("Created by zigi (zigisoftware).");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - 10.0f);

    ImGui::TextColored(Colors::Red, "WHY WAS THIS CREATED?");
    ImGui::TextUnformatted("Because I got tired of the base game's complete lack of QoL features and dealing with absolute garbage APIs written by amateurs. If the engine doesn't do what you want, you don't cry about it—you just build your own pipeline and inject that shit straight into the game's RAM. My life motto says it all: \"Not giving a f*** brings rewards, and improvisation is the key to success.\"");
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::TextColored(Colors::Red, "THINGS I ABSOLUTELY HATE (THE HALL OF SHAME):");
    ImGui::BulletText("Useless dynamic scrollbars ruining perfectly static UI designs.");
    ImGui::BulletText("Bloatware, badly optimized code, and junior devs who copy-paste everything.");
    ImGui::BulletText("Explaining for the hundredth time how to properly unpack a fucking library.");
    ImGui::BulletText("Overcomplicated frameworks created for literal cwels who can't handle bare-metal memory allocation.");
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::TextColored(Colors::Green, "SPECIAL THANKS:");
    ImGui::TextUnformatted("To my mixed-breed dog, Panini, for keeping me sane while watching hundreds of C++ compiler errors fly across my screen.");
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();


    ImGui::PopTextWrapPos();
    ImGui::EndChild();
}

void DrawSettingsTab(AppSettings& settings, const std::string& configPath) {
    ImGui::BeginChild("##settings_tracker", ImVec2(0, 0), false);
    SectionHeader("SETTINGS");

    ImGui::Spacing();
    ImGui::TextColored(Colors::White, "Updates");
    ImGui::Separator();
    ImGui::Spacing();

    bool isNewVersion = !g_UpdateVersion.empty() && 
                        (g_UpdateVersion[0] == 'v' || std::isdigit(g_UpdateVersion[0])) && 
                        g_UpdateVersion.find("error") == std::string::npos &&
                        g_UpdateVersion.find("Exception") == std::string::npos &&
                        g_UpdateVersion.find("Invoke-RestMethod") == std::string::npos &&
                        g_UpdateVersion != LC_MANAGER_VERSION && 
                        g_UpdateVersion != "v" LC_MANAGER_VERSION &&
                        std::string("v") + g_UpdateVersion != LC_MANAGER_VERSION;

    if (isNewVersion) {
        ImGui::TextColored(Colors::Red, "A new version of LC Manager is available: %s", g_UpdateVersion.c_str());
        ImGui::TextColored(Colors::Muted, "Click below to open the GitHub Releases page to download it.");
        if (ImGui::Button("Open GitHub Releases", ImVec2(200, 30))) {
            ShellExecuteA(NULL, "open", "https://github.com/DEVZiGi/lc_manager/releases/latest", NULL, NULL, SW_SHOWNORMAL);
        }
    } else {
        ImGui::TextColored(Colors::Green, "You are using the latest version (%s).", LC_MANAGER_VERSION);
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::TextColored(Colors::White, "Save Data Management");
    ImGui::Separator();
    ImGui::Spacing();

    static char backupName[128] = "Backup_Pre_Day_46";
    ImGui::InputText("Backup Name", backupName, IM_ARRAYSIZE(backupName));
    
    if (ImGui::Button("Create Backup ZIP", ImVec2(200, 30))) {
        std::string nameStr(backupName);
        if (nameStr.empty()) nameStr = "Backup";
        
        std::string srcDir = std::string(getenv("USERPROFILE")) + "\\AppData\\LocalLow\\Project_Moon\\Lobotomy";
        std::string destDir = std::string(getenv("USERPROFILE")) + "\\AppData\\LocalLow\\zigisoftware\\lc_manager";
        
        std::system(("mkdir \"" + destDir + "\" >nul 2>&1").c_str());
        
        std::string pFile1 = srcDir + "\\etc170808.dat";
        std::string pFile2 = srcDir + "\\saveData170808.dat";
        std::string pFile3 = srcDir + "\\saveGlobal170808.dat";
        
        std::string destZip = destDir + "\\" + nameStr + ".zip";
        
        std::string psCmd = "powershell -NoProfile -Command \"Compress-Archive -Path '" + pFile1 + "', '" + pFile2 + "', '" + pFile3 + "' -DestinationPath '" + destZip + "' -Force\"";
        
        int result = std::system(psCmd.c_str());
        if (result == 0) {
            std::string openCmd = "explorer \"" + destDir + "\"";
            std::system(openCmd.c_str());
        }
    }

    ImGui::EndChild();
}