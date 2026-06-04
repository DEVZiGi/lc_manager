#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <initializer_list>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>
#include <dwmapi.h>

#include <nlohmann/json.hpp>
#include "lc_manager_database/database.h"
#include "exp_calculator.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define LC_MANAGER_VERSION "v0.1"

namespace fs = std::filesystem;
using json = nlohmann::json;

struct AppSettings {
    bool autoInject = false;
};

DWORD GetProcessIdByName(const std::string& processName) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &processEntry)) {
        do {
            if (processName == processEntry.szExeFile) {
                CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    CloseHandle(snapshot);
    return 0;
}

std::string GetLocalLowPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path) + "\\AppData\\LocalLow\\zigisoftware\\lc_manager";
    }
    return "";
}

std::string GetExeDirectory() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string fullPath(buffer);
    return fullPath.substr(0, fullPath.find_last_of("\\/"));
}

void SetupDirectories(const std::string& basePath) {
    if (basePath.empty()) return;
    std::vector<std::string> dirs = { basePath, basePath + "\\data", basePath + "\\database" };
    for (const auto& dir : dirs) {
        if (!fs::exists(dir)) fs::create_directories(dir);
    }
}

void SetupStaticDatabase(const std::string& dbFolder) {
    std::string exeDir = GetExeDirectory();
    std::vector<std::string> jsonFiles = { "abno_db.json", "equip_db.json" };
    fs::create_directories(dbFolder);
    for (const auto& file : jsonFiles) {
        std::string sourceFile = exeDir + "\\" + file;
        std::string targetFile = dbFolder + "\\" + file;
        if (fs::exists(sourceFile) && !fs::exists(targetFile)) {
            try { fs::copy(sourceFile, targetFile, fs::copy_options::overwrite_existing); }
            catch (...) {}
        }
    }
}

void LoadConfig(AppSettings& settings, const std::string& configPath) {
    if (fs::exists(configPath)) {
        try {
            std::ifstream file(configPath);
            json j = json::parse(file);
            settings.autoInject = j.value("autoInject", false);
        } catch (...) {}
    }
}

void SaveConfig(const AppSettings& settings, const std::string& configPath) {
    try {
        json j;
        j["autoInject"] = settings.autoInject;
        std::ofstream file(configPath);
        file << j.dump(4);
    } catch (...) {}
}

bool CreateDesktopShortcut() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char desktopPath[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) return false;
    std::string linkPath = std::string(desktopPath) + "\\LC Manager.lnk";
    HRESULT hres;
    IShellLink* psl;
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        psl->SetPath(exePath);
        psl->SetDescription("LC Manager - QoL APP by zigisoftware");
        psl->SetIconLocation(exePath, 0);
        IPersistFile* ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres)) {
            WCHAR wsz[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, linkPath.c_str(), -1, wsz, MAX_PATH);
            hres = ppf->Save(wsz, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return SUCCEEDED(hres);
}

namespace Colors {
    static const ImVec4 Accent      = { 0.98f, 0.73f, 0.01f, 1.00f };
    static const ImVec4 AccentSoft  = { 0.45f, 0.35f, 0.05f, 1.00f };
    static const ImVec4 Link        = { 0.98f, 0.80f, 0.20f, 1.00f };
    static const ImVec4 Green       = { 0.20f, 0.80f, 0.40f, 1.00f };
    static const ImVec4 GreenBg     = { 0.05f, 0.20f, 0.10f, 1.00f };
    static const ImVec4 GreenText   = { 0.40f, 0.90f, 0.50f, 1.00f };
    static const ImVec4 Yellow      = { 0.95f, 0.85f, 0.25f, 1.00f };
    static const ImVec4 YellowBg    = { 0.25f, 0.22f, 0.05f, 1.00f };
    static const ImVec4 Red         = { 0.95f, 0.25f, 0.25f, 1.00f };
    static const ImVec4 RedBg       = { 0.25f, 0.05f, 0.05f, 1.00f };
    static const ImVec4 RedBtn      = { 0.40f, 0.10f, 0.10f, 1.00f };
    static const ImVec4 RedBtnH     = { 0.60f, 0.15f, 0.15f, 1.00f };
    static const ImVec4 RedBtnA     = { 0.80f, 0.20f, 0.20f, 1.00f };
    static const ImVec4 Muted       = { 0.60f, 0.62f, 0.65f, 1.00f };
    static const ImVec4 Dim         = { 0.40f, 0.42f, 0.45f, 1.00f };
    static const ImVec4 White       = { 0.95f, 0.95f, 0.98f, 1.00f };
    static const ImVec4 Panel       = { 0.05f, 0.05f, 0.06f, 1.00f };
    static const ImVec4 PanelSoft   = { 0.09f, 0.09f, 0.10f, 1.00f };
    static const ImVec4 Border      = { 0.25f, 0.25f, 0.28f, 1.00f };
    static const ImVec4 TableHead   = { 0.10f, 0.10f, 0.11f, 1.00f };
}


static std::string NormalizeText(const std::string& input) {
    std::string out = input;
    std::string from[] = {"ą", "ć", "ę", "ł", "ń", "ó", "ś", "ź", "ż", "Ą", "Ć", "Ę", "Ł", "Ń", "Ó", "Ś", "Ź", "Ż"};
    std::string to[] = {"a", "c", "e", "l", "n", "o", "s", "z", "z", "a", "c", "e", "l", "n", "o", "s", "z", "z"};
    for (int i = 0; i < 18; ++i) {
        size_t pos = 0;
        while ((pos = out.find(from[i], pos)) != std::string::npos) {
            out.replace(pos, from[i].length(), to[i]);
            pos += to[i].length();
        }
    }
    for (char& c : out) {
        c = std::tolower(static_cast<unsigned char>(c));
    }
    return out;
}

static void SectionHeader(const char* label) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 5.0f));
    ImGui::TextColored(Colors::Accent, "%s", label);
    ImGui::PopStyleVar();
    ImGui::PushStyleColor(ImGuiCol_Separator, Colors::AccentSoft);
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}



static void DrawMetricPill(const char* label, const std::string& value, const ImVec4& accent = Colors::Accent) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    std::string text = std::string(label) + ": " + value;
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    ImVec2 pad(10.0f, 5.0f);
    ImVec2 size(textSize.x + pad.x * 2.0f, textSize.y + pad.y * 2.0f);

    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::ColorConvertFloat4ToU32(Colors::PanelSoft), 2.0f);
    drawList->AddRectFilled(pos, ImVec2(pos.x + 3.0f, pos.y + size.y), ImGui::ColorConvertFloat4ToU32(accent), 2.0f, ImDrawFlags_RoundCornersLeft);
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::ColorConvertFloat4ToU32(Colors::Border), 2.0f);
    ImGui::SetCursorScreenPos(ImVec2(pos.x + pad.x, pos.y + pad.y));
    ImGui::TextColored(Colors::White, "%s", text.c_str());
    ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x, pos.y));
    ImGui::Dummy(size);
}

static void DrawStatusRow(const char* label, bool active, const char* onText, const char* offText = "OFFLINE") {
    ImGui::TextColored(Colors::Muted, "%-10s", label);
    ImGui::SameLine();
    if (active) {
        ImGui::TextColored(Colors::Green, "[ OK ]");
        ImGui::SameLine();
        ImGui::TextColored(Colors::GreenText, "%s", onText);
    } else {
        ImGui::TextColored(Colors::Yellow, "[ IDLE ]");
        ImGui::SameLine();
        ImGui::TextColored(Colors::Muted, "%s", offText);
    }
}

static void DrawTopBar(DWORD pid, bool injected, bool databaseReady, bool liveDataReady) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::BeginChild("##topbar", ImVec2(0, 48), true, flags);
    ImGui::TextColored(Colors::Accent, "zigisoftware");
    ImGui::SameLine();
    ImGui::TextColored(Colors::Dim, "/");
    ImGui::SameLine();
    ImGui::TextColored(Colors::White, "LC Manager");
    ImGui::SameLine();
    ImGui::TextColored(Colors::Muted, LC_MANAGER_VERSION);

    float right = ImGui::GetContentRegionAvail().x - 120.0f;
    if (right > 20.0f) ImGui::SameLine(ImGui::GetCursorPosX() + right);
    else ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, Colors::Red);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("FORCE CLOSE", ImVec2(100.0f, 24.0f))) {
        system("taskkill /F /IM LobotomyCorp.exe /T");
    }
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::Spacing();
}

static std::string ToUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

static bool HasText(const std::string& value) {
    return !value.empty() && value != "-" && value != "null";
}

static const json* FindAny(const json& object, std::initializer_list<const char*> keys) {
    if (!object.is_object()) return nullptr;

    for (const char* key : keys) {
        auto it = object.find(key);
        if (it != object.end() && !it->is_null()) return &(*it);
    }
    return nullptr;
}

static std::string JsonToString(const json* value, const std::string& fallback = "-") {
    if (value == nullptr || value->is_null()) return fallback;

    if (value->is_string()) {
        std::string text = value->get<std::string>();
        return text.empty() ? fallback : text;
    }
    if (value->is_boolean()) return value->get<bool>() ? "true" : "false";
    if (value->is_number_integer() || value->is_number_unsigned()) return std::to_string(value->get<long long>());
    if (value->is_number_float()) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(1) << value->get<double>();
        return out.str();
    }

    return fallback;
}

static std::string JsonToString(const json& object, std::initializer_list<const char*> keys, const std::string& fallback = "-") {
    return JsonToString(FindAny(object, keys), fallback);
}

static int JsonToInt(const json* value, int fallback = 0) {
    if (value == nullptr || value->is_null()) return fallback;
    try {
        if (value->is_number_integer() || value->is_number_unsigned()) return value->get<int>();
        if (value->is_number_float()) return static_cast<int>(std::round(value->get<double>()));
        if (value->is_string()) return std::stoi(value->get<std::string>());
    } catch (...) {}
    return fallback;
}

static int JsonToInt(const json& object, std::initializer_list<const char*> keys, int fallback = 0) {
    return JsonToInt(FindAny(object, keys), fallback);
}

static double JsonToDouble(const json* value, double fallback = 0.0) {
    if (value == nullptr || value->is_null()) return fallback;
    try {
        if (value->is_number()) return value->get<double>();
        if (value->is_string()) return std::stod(value->get<std::string>());
    } catch (...) {}
    return fallback;
}

static double JsonToDouble(const json& object, std::initializer_list<const char*> keys, double fallback = 0.0) {
    return JsonToDouble(FindAny(object, keys), fallback);
}

static bool JsonToBool(const json& object, std::initializer_list<const char*> keys, bool fallback = false) {
    const json* value = FindAny(object, keys);
    if (value == nullptr || value->is_null()) return fallback;
    if (value->is_boolean()) return value->get<bool>();
    std::string text = ToUpper(JsonToString(value, ""));
    if (text == "TRUE" || text == "YES" || text == "1") return true;
    if (text == "FALSE" || text == "NO" || text == "0") return false;
    return fallback;
}

static bool LoadLiveData(const std::string& liveDataPath, json& out, std::string& error) {
    if (!fs::exists(liveDataPath)) {
        error = "Waiting for live_data.json";
        return false;
    }

    try {
        std::ifstream file(liveDataPath);
        out = json::parse(file, nullptr, false);
        if (out.is_discarded()) {
            error = "live_data.json is being written; retrying";
            return false;
        }

        std::string status = JsonToString(out, {"status"}, "");
        if (status == "ERROR") {
            error = JsonToString(out, {"message"}, "Pipeline error");
            return true;
        }

        error.clear();
        return true;
    } catch (const std::exception& ex) {
        error = ex.what();
        return false;
    } catch (...) {
        error = "Unknown live data read error";
        return false;
    }
}

static void DrawNoLiveData(const std::string& error) {
    ImGui::BeginChild("##no_live_data", ImVec2(0, 0), true);
    SectionHeader("PIPELINE");
    ImGui::TextColored(Colors::Muted, "Live game snapshot is not ready yet.");
    ImGui::Spacing();
    ImGui::TextColored(Colors::Muted, "Reason: %s", error.empty() ? "unknown" : error.c_str());
    ImGui::Spacing();
    ImGui::TextColored(Colors::Muted, "Start the game, inject the module, then wait for the next data tick.");
    ImGui::EndChild();
}

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
    if (upper.find("MALKUTH") != std::string::npos || upper.find("CONTROL") != std::string::npos) return "Control Team";
    if (upper.find("YESOD") != std::string::npos || upper.find("INFORMATION") != std::string::npos) return "Information Team";
    if (upper.find("HOD") != std::string::npos || upper.find("TRAINING") != std::string::npos) return "Training Team";
    if (upper.find("NETZACH") != std::string::npos || upper.find("SAFETY") != std::string::npos || upper.find("SECURITY") != std::string::npos) return "Safety Team";
    if (upper.find("TIPHERETH") != std::string::npos || upper.find("CENTRAL") != std::string::npos) return "Central Command";
    if (upper.find("CHESED") != std::string::npos || upper.find("WELFARE") != std::string::npos) return "Welfare Team";
    if (upper.find("GEBURA") != std::string::npos || upper.find("DISCIPLINARY") != std::string::npos) return "Disciplinary Team";
    if (upper.find("BINAH") != std::string::npos || upper.find("EXTRACTION") != std::string::npos) return "Extraction Team";
    if (upper.find("HOKMA") != std::string::npos || upper.find("RECORD") != std::string::npos) return "Records Team";

    try {
        int index = std::stoi(raw);
        static const char* departments[] = {
            "Control Team", "Information Team", "Training Team", "Safety Team",
            "Central Command", "Welfare Team", "Disciplinary Team", "Extraction Team", "Records Team"
        };
        if (index >= 0 && index < 9) return departments[index];
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

static void DrawMetric(const char* label, const std::string& value) {
    ImGui::TextColored(Colors::Muted, "%s", label);
    ImGui::SameLine(180.0f);
    ImGui::TextColored(Colors::White, "%s", value.c_str());
}

static void DrawAgentsTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError, DatabaseManager& db) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
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
        if (ImGui::BeginTable(tableId, 10, flags, ImVec2(0, tableHeight))) {
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 130.0f);
            ImGui::TableSetupColumn("Unit", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Current work", ImGuiTableColumnFlags_WidthFixed, 190.0f);
            ImGui::TableSetupColumn("Overall", ImGuiTableColumnFlags_WidthFixed, 76.0f);
            ImGui::TableSetupColumn("Fortitude", ImGuiTableColumnFlags_WidthFixed, 88.0f);
            ImGui::TableSetupColumn("Prudence", ImGuiTableColumnFlags_WidthFixed, 88.0f);
            ImGui::TableSetupColumn("Temperance", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Justice", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Gifts", ImGuiTableColumnFlags_WidthFixed, 58.0f);
            ImGui::TableSetupColumn("HP / SP", ImGuiTableColumnFlags_WidthFixed, 96.0f);
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
                ImGui::TableSetColumnIndex(9);
                ImGui::Text("%d/%d", JsonToInt(*agent, {"hp"}, 0), JsonToInt(*agent, {"sp"}, 0));
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

    int typeId = JsonToInt(item, {"typeId", "equipmentId", "id"}, -1);
    std::string rowKey = typeId >= 0 ? ("type:" + std::to_string(typeId)) : ("name:" + EquipmentName(db, item));
    std::string instanceId = JsonToString(item, {"instanceId"}, "");
    std::string instanceKey = HasText(instanceId) ? ("inst:" + instanceId) : "";

    InventoryRow& row = rows[rowKey];
    row.typeId = typeId;
    row.name = EquipmentName(db, item);
    row.type = EquipmentType(db, item);
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

static void DrawInventoryTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError, DatabaseManager& db) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
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

    static char searchFilter[128] = "";
    ImGui::InputText("Search", searchFilter, 128);
    ImGui::Spacing();

    std::string searchStr = searchFilter;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

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
                case 7: delta = left.typeId - right.typeId; break;
                default: break;
            }
            if (delta > 0) return sort_spec->SortDirection == ImGuiSortDirection_Ascending;
            if (delta < 0) return sort_spec->SortDirection == ImGuiSortDirection_Descending;
        }
        return left.name < right.name;
    };

    auto renderTable = [&](const char* id, std::vector<InventoryRow>& tableRows) {
        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable;
        if (ImGui::BeginTable(id, 8, flags, ImVec2(0, 300))) {
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthFixed, 220.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed, 58.0f);
            ImGui::TableSetupColumn("Assigned", ImGuiTableColumnFlags_WidthFixed, 74.0f);
            ImGui::TableSetupColumn("Free", ImGuiTableColumnFlags_WidthFixed, 58.0f);
            ImGui::TableSetupColumn("Gift stats", ImGuiTableColumnFlags_WidthFixed, 155.0f);
            ImGui::TableSetupColumn("Assigned to", ImGuiTableColumnFlags_WidthFixed, 240.0f);
            ImGui::TableSetupColumn("Type ID", ImGuiTableColumnFlags_WidthFixed, 70.0f);
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
                    if (row.stats.hp != 0 || row.stats.sp != 0 || row.stats.ws != 0 || row.stats.as != 0) {
                        ImGui::SetTooltip("Stat Bonuses:\nHP: %+d\nSP: %+d\nWork Speed: %+d\nAttack Speed: %+d",
                            row.stats.hp, row.stats.sp, row.stats.ws, row.stats.as);
                    } else if (row.type == "WEAPON" || row.type == "ARMOR") {
                        ImGui::SetTooltip("Advanced stats for %s are not extracted in this version.", row.type.c_str());
                    }
                }
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(Colors::Yellow, "%s", row.type.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", row.total);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", row.assigned);
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%d", row.total - row.assigned);
                ImGui::TableSetColumnIndex(5);
                if (row.type == "GIFT") {
                    ImGui::TextColored(Colors::Green, "HP:%d SP:%d WS:%d AS:%d", row.stats.hp, row.stats.sp, row.stats.ws, row.stats.as);
                } else {
                    ImGui::TextColored(Colors::Muted, "-");
                }
                ImGui::TableSetColumnIndex(6);
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
                ImGui::TableSetColumnIndex(7);
                ImGui::TextColored(Colors::Muted, "%d", row.typeId);
            }
            ImGui::EndTable();
        }
    };

    if (ImGui::CollapsingHeader("WEAPONS & ARMORS", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderTable("##inventory_table_equip", equipmentRows);
        ImGui::Spacing();
    }
    if (ImGui::CollapsingHeader("E.G.O GIFTS", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderTable("##inventory_table_gift", giftRows);
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

static void DrawOrdealTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
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
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 42.0f);
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

static void DrawExpTrackerTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
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
    ImGui::Checkbox("Show Only Almost Leveled Up (>95%)", &showOnlyAlmostLeveled);
    ImGui::Spacing();

    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##exp_table", 5, flags, ImVec2(0, 0))) {
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableSetupColumn("Agent Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Fortitude (R)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Prudence (W)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Temperance (B)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Justice (P)", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const json* agent : agents) {
            double expHp = JsonToDouble(*agent, {"expHp"}, 0.0);
            int lvlHp = StatTier(JsonToInt(*agent, {"fortitude"}, 0));

            double expMental = JsonToDouble(*agent, {"expMental"}, 0.0);
            int lvlMental = StatTier(JsonToInt(*agent, {"prudence"}, 0));

            double expWork = JsonToDouble(*agent, {"expWork"}, 0.0);
            int lvlWork = StatTier(JsonToInt(*agent, {"temperance"}, 0));

            double expBattle = JsonToDouble(*agent, {"expBattle"}, 0.0);
            int lvlBattle = StatTier(JsonToInt(*agent, {"justice"}, 0));

            if (showOnlyAlmostLeveled) {
                bool almostLeveled = false;
                if (expHp >= 95.0 && lvlHp < 6) almostLeveled = true;
                if (expMental >= 95.0 && lvlMental < 6) almostLeveled = true;
                if (expWork >= 95.0 && lvlWork < 6) almostLeveled = true;
                if (expBattle >= 95.0 && lvlBattle < 6) almostLeveled = true;

                if (!almostLeveled) continue;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", JsonToString(*agent, {"name"}, "Unnamed").c_str());

            // Fortitude
            ImGui::TableSetColumnIndex(1);
            if (lvlHp < 6) ImGui::TextColored(Colors::Yellow, "%s (%.1f%%)", TierLabel(lvlHp), expHp);
            else ImGui::TextColored(Colors::Green, "EX (%.1f)", expHp);

            // Prudence
            ImGui::TableSetColumnIndex(2);
            if (lvlMental < 6) ImGui::TextColored(Colors::Yellow, "%s (%.1f%%)", TierLabel(lvlMental), expMental);
            else ImGui::TextColored(Colors::Green, "EX (%.1f)", expMental);

            // Temperance
            ImGui::TableSetColumnIndex(3);
            if (lvlWork < 6) ImGui::TextColored(Colors::Yellow, "%s (%.1f%%)", TierLabel(lvlWork), expWork);
            else ImGui::TextColored(Colors::Green, "EX (%.1f)", expWork);

            // Justice
            ImGui::TableSetColumnIndex(4);
            if (lvlBattle < 6) ImGui::TextColored(Colors::Yellow, "%s (%.1f%%)", TierLabel(lvlBattle), expBattle);
            else ImGui::TextColored(Colors::Green, "EX (%.1f)", expBattle);
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

static void DrawEnergyBreakdown(const json& liveData) {
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
    if (ImGui::BeginTable("##observation_table", 5, flags, ImVec2(0, 190))) {
        ImGui::TableSetupColumn("Abnormality");
        ImGui::TableSetupColumn("Code");
        ImGui::TableSetupColumn("Risk");
        ImGui::TableSetupColumn("Qliphoth");
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();

        for (const json* abnormality : abnormalities) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            std::string name = AbnormalityName(db, *abnormality);
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::Link);
            if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                std::string urlName = name;
                for (char& c : urlName) if (c == ' ') c = '_';
                std::string url = "https://lobotomycorp.fandom.com/wiki/" + urlName;
                ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
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
            std::string worker = JsonToString(*abnormality, {"worker"}, "");
            if (state == "ESCAPE") {
                ImGui::TextColored(Colors::Red, "ESCAPED");
            } else if (HasText(worker)) {
                ImGui::TextColored(Colors::Yellow, "Working (%s)", worker.c_str());
            } else {
                ImGui::Text("%s", state.c_str());
            }

        }
        ImGui::EndTable();
    }
}

static void DrawDetailedInfoTab(const json& liveData, bool liveDataLoaded, const std::string& liveDataError, DatabaseManager& db) {
    if (!liveDataLoaded) {
        DrawNoLiveData(liveDataError);
        return;
    }

    ImGui::BeginChild("##detailed_info_tab", ImVec2(0, 0), false);
    SectionHeader("DETAILED INFO");

    if (ImGui::CollapsingHeader("Facility and energy", ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawEnergyBreakdown(liveData);
        ImGui::Spacing();
    }

    std::vector<const json*> agents = CollectAgents(liveData, true);
    std::vector<const json*> abnormalities = CollectAbnormalities(liveData);



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

static void DrawExtractionTab(const json& liveData, DatabaseManager& db) {
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
        for (const auto& choice : *choices) {
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
                std::string urlName = name;
                for (char& c : urlName) if (c == ' ') c = '_';
                std::string url = "https://lobotomycorp.fandom.com/wiki/" + urlName;
                ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
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

static void DrawInfoTab() {
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

bool InjectMono() {
    std::string exeDir = GetExeDirectory();
    std::string smiPath = exeDir + "\\smi.exe";
    std::string dllPath = exeDir + "\\lc_manager_sdk.dll";

    if (!fs::exists(smiPath) || !fs::exists(dllPath)) return false;

    // Czysta komenda, bez cmd.exe
    std::string command = "inject -p LobotomyCorp -a \"" + dllPath + "\" -n lc_manager_sdk -c Loader -m Init";

    SHELLEXECUTEINFOA shExecInfo = {0};
    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.hwnd = NULL;
    shExecInfo.lpVerb = "open";
    shExecInfo.lpFile = smiPath.c_str();
    shExecInfo.lpParameters = command.c_str();
    shExecInfo.lpDirectory = NULL;
    shExecInfo.nShow = SW_HIDE;
    shExecInfo.hInstApp = NULL;

    if (ShellExecuteExA(&shExecInfo)) {
        WaitForSingleObject(shExecInfo.hProcess, 3000);
        CloseHandle(shExecInfo.hProcess);
        return true;
    }
    return false;
}

int main() {
    CoInitialize(NULL);

    std::string lcManagerPath = GetLocalLowPath();
    std::string dbFolder      = lcManagerPath + "\\database";
    std::string liveDataPath  = lcManagerPath + "\\data\\live_data.json";
    std::string configPath    = lcManagerPath + "\\config.json";

    SetupDirectories(lcManagerPath);
    SetupStaticDatabase(dbFolder);

    DatabaseManager dbManager;
    bool dbLoaded = dbManager.Initialize(dbFolder);

    AppSettings settings;
    LoadConfig(settings, configPath);

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1050, 750, "LC Manager " LC_MANAGER_VERSION, nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    HWND hwnd = glfwGetWin32Window(window);

    HICON hIcon = LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(101));
    if (hIcon) {
        SendMessageA(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 2.0f;
    style.FrameRounding     = 0.0f;
    style.TabRounding       = 2.0f;
    style.GrabRounding      = 0.0f;
    style.PopupRounding     = 2.0f;
    style.ScrollbarRounding = 0.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.ScrollbarSize     = 12.0f;

    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(10.0f,  6.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2( 6.0f,  6.0f);
    style.CellPadding       = ImVec2( 6.0f,  6.0f);
    style.TabBarBorderSize  = 1.0f;

    style.Colors[ImGuiCol_Text]               = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]       = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]           = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_ChildBg]            = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    style.Colors[ImGuiCol_PopupBg]            = ImVec4(0.08f, 0.08f, 0.09f, 0.98f);
    style.Colors[ImGuiCol_Border]             = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]            = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]      = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]            = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    style.Colors[ImGuiCol_MenuBarBg]          = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.02f, 0.02f, 0.03f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]          = Colors::Accent;
    style.Colors[ImGuiCol_SliderGrab]         = Colors::Accent;
    style.Colors[ImGuiCol_SliderGrabActive]   = Colors::Accent;
    style.Colors[ImGuiCol_Button]             = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]      = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]       = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_Header]             = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]      = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]       = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_Separator]          = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]   = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive]    = Colors::Accent;
    style.Colors[ImGuiCol_ResizeGrip]         = ImVec4(0.98f, 0.73f, 0.01f, 0.20f);
    style.Colors[ImGuiCol_ResizeGripHovered]  = ImVec4(0.98f, 0.73f, 0.01f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]   = ImVec4(0.98f, 0.73f, 0.01f, 0.95f);
    style.Colors[ImGuiCol_Tab]                = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_TabHovered]         = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TabActive]          = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]          = Colors::Accent;
    style.Colors[ImGuiCol_PlotLinesHovered]   = ImVec4(1.00f, 0.85f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]      = Colors::Accent;
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.85f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_TableHeaderBg]      = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_TableBorderStrong]  = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_TableBorderLight]   = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TableRowBg]         = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
    style.Colors[ImGuiCol_TableRowBgAlt]      = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool        isInjected      = false;
    bool        shortcutCreated = false;
    bool        configSaved     = false;
    bool        liveDataLoaded  = false;
    DWORD       lcProcessId     = 0;
    int         frameCounter    = 0;
    json        liveData;
    std::string liveDataError = "Waiting for live_data.json";

    ImGuiWindowFlags staticFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (frameCounter % 60 == 0) {
            lcProcessId = GetProcessIdByName("LobotomyCorp.exe");
            if (lcProcessId == 0) isInjected = false;
        }

        if (frameCounter % 30 == 0) {
            json newLiveData;
            std::string newError;
            if (LoadLiveData(liveDataPath, newLiveData, newError)) {
                liveData = newLiveData;
                liveDataLoaded = true;
                liveDataError = newError;
            } else {
                liveDataError = newError;
            }
        }
        frameCounter++;

        if (settings.autoInject && lcProcessId > 0 && !isInjected) {
            if (InjectMono()) {
                isInjected = true;
            }
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("##root", nullptr,
            ImGuiWindowFlags_NoDecoration   | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove         | ImGuiWindowFlags_NoBringToFrontOnFocus);

        DrawTopBar(lcProcessId, isInjected, dbLoaded, liveDataLoaded);

        if (liveDataLoaded) {
            const json* clerks = FindAny(liveData, {"clerks"});
            if (clerks && clerks->is_object()) {
                std::vector<std::string> alerts;
                for (auto it = clerks->begin(); it != clerks->end(); ++it) {
                    int total = JsonToInt(it.value(), {"total"}, 0);
                    int alive = JsonToInt(it.value(), {"alive"}, 0);
                    if (total > 0 && (float)alive / total < 0.3f) {
                        alerts.push_back(it.key());
                    }
                }

                if (!alerts.empty()) {
                    bool flash = ((ImGui::GetTime() * 4.0f) - floor(ImGui::GetTime() * 4.0f) > 0.5f);
                    if (flash) ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::RedBg);

                    ImGui::BeginChild("##sephira_alerts", ImVec2(0, 36), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    if (flash) ImGui::PushStyleColor(ImGuiCol_Text, Colors::Red);
                    else ImGui::PushStyleColor(ImGuiCol_Text, Colors::Yellow);

                    ImGui::Text("SEPHIRA ALERT: Clerk survival rate critical (<30%%) in:");
                    for (const std::string& dept : alerts) {
                        ImGui::SameLine();
                        ImGui::Text("[%s]", dept.c_str());
                    }
                    ImGui::PopStyleColor();
                    ImGui::EndChild();

                    if (flash) ImGui::PopStyleColor();
                    ImGui::Spacing();
                }
            }
        }

        if (ImGui::BeginTabBar("MainTabs")) {


            if (ImGui::BeginTabItem("OVERVIEW")) {
                float panelW = ImGui::GetContentRegionAvail().x;
                float halfW  = panelW * 0.5f - style.ItemSpacing.x * 0.5f;

                ImGui::BeginChild("##status", ImVec2(panelW, 115), true, staticFlags);
                SectionHeader("STATUS");

                ImGui::BeginGroup();
                {
                    char pidBuf[48] = "OFFLINE";
                    if (lcProcessId > 0)
                        snprintf(pidBuf, sizeof(pidBuf), "RUNNING  (PID: %lu)", lcProcessId);
                    DrawStatusRow("GAME",     lcProcessId > 0, pidBuf);
                    DrawStatusRow("DATABASE", dbLoaded, "ONLINE", "MISSING JSONS");
                }
                ImGui::EndGroup();

                ImGui::SameLine(halfW);

                ImGui::BeginGroup();
                {
                    DrawStatusRow("MODULE",  isInjected, "ACTIVE", "IDLE");
                }
                ImGui::EndGroup();

                ImGui::EndChild();
                ImGui::Spacing();

                ImGui::BeginChild("##ops", ImVec2(panelW, 100), true, staticFlags);
                SectionHeader("OPERATIONS");

                if (lcProcessId > 0 && !isInjected) {
                    if (ImGui::Button("INJECT MODULE", ImVec2(200, 38))) {
                        if (InjectMono()) {
                            isInjected = true;
                        }
                    }
                } else if (isInjected) {
                    ImGui::Spacing();
                    ImGui::TextColored(Colors::Green, "  >>  PIPELINE OPEN  |  DATA SYNCING  <<");
                } else {
                    ImGui::Spacing();
                    ImGui::TextColored(Colors::Muted, "  Awaiting LobotomyCorp.exe to start...");
                }
                ImGui::EndChild();
                ImGui::Spacing();

                ImGui::BeginChild("##prefs", ImVec2(panelW, 85), true, staticFlags);
                SectionHeader("PREFERENCES");

                ImGui::Checkbox("Auto-inject on detection", &settings.autoInject);

                ImGui::EndChild();
                ImGui::Spacing();

                ImGui::BeginChild("##mgmt", ImVec2(panelW, 0), true, staticFlags);
                SectionHeader("APP MANAGEMENT");

                if (ImGui::Button("SAVE CONFIG", ImVec2(220, 34))) {
                    SaveConfig(settings, configPath);
                    configSaved = true;
                }
                if (configSaved) {
                    ImGui::SameLine();
                    ImGui::TextColored(Colors::Green, "Settings saved!");
                }

                ImGui::Spacing();
                if (ImGui::Button("CREATE DESKTOP SHORTCUT", ImVec2(220, 34)))
                    if (CreateDesktopShortcut()) shortcutCreated = true;

                if (shortcutCreated) {
                    ImGui::SameLine();
                    ImGui::TextColored(Colors::Green, "Created!");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button,        Colors::RedBtn);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::RedBtnH);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Colors::RedBtnA);
                if (ImGui::Button("UNINSTALL MANAGER", ImVec2(180, 34))) {
                    std::string rootFolder = std::string(getenv("USERPROFILE")) + "\\AppData\\LocalLow\\zigisoftware";
                    if (fs::exists(rootFolder)) fs::remove_all(rootFolder);
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                ImGui::TextColored(Colors::Muted, "<- Deletes config, database, and LocalLow folders");

                ImGui::Spacing();

                if (ImGui::Button("EXIT", ImVec2(180, 34)))
                    glfwSetWindowShouldClose(window, GLFW_TRUE);

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            bool pipelineReady = (isInjected && lcProcessId > 0);
            if (pipelineReady) {
                if (ImGui::BeginTabItem("AGENTS")) {
                    DrawAgentsTab(liveData, liveDataLoaded, liveDataError, dbManager);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("INVENTORY")) {
                    DrawInventoryTab(liveData, liveDataLoaded, liveDataError, dbManager);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("TODAY'S ORDEAL")) {
                    DrawOrdealTab(liveData, liveDataLoaded, liveDataError);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("EXP TRACKER")) {
                    DrawExpTrackerTab(liveData, liveDataLoaded, liveDataError);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("DETAILED INFO")) {
                    DrawDetailedInfoTab(liveData, liveDataLoaded, liveDataError, dbManager);
                    ImGui::EndTabItem();
                }
            }

            const json* extractChoices = FindAny(liveData, {"extractionChoices"});
            bool hasExtraction = extractChoices != nullptr && extractChoices->is_array() && extractChoices->size() > 0;

            bool highlight = hasExtraction && ((ImGui::GetTime() * 2.0f) - floor(ImGui::GetTime() * 2.0f) > 0.5f);
            if (highlight) ImGui::PushStyleColor(ImGuiCol_Text, Colors::Green);
            if (ImGui::BeginTabItem("EXTRACTION")) {
                if (highlight) ImGui::PopStyleColor();
                DrawExtractionTab(liveData, dbManager);
                ImGui::EndTabItem();
            } else {
                if (highlight) ImGui::PopStyleColor();
            }

            if (ImGui::BeginTabItem("INFO")) {
                DrawInfoTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.04f, 0.04f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Clean up live_data.json on exit so it doesn't cause stale data issues on next launch
    if (fs::exists(liveDataPath)) {
        try { fs::remove(liveDataPath); } catch (...) {}
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    CoUninitialize();

    if (fs::exists(liveDataPath)) {
        try { fs::remove(liveDataPath); } catch (...) {}
    }

    return 0;
}
