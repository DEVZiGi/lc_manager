#include "ui_components.h"

#define LC_MANAGER_VERSION "v0.2"

namespace Colors {
    const ImVec4 Accent      = { 0.54f, 0.17f, 0.89f, 1.00f }; // BlueViolet
    const ImVec4 AccentSoft  = { 0.65f, 0.35f, 0.95f, 1.00f }; // Lighter Violet
    const ImVec4 Link        = { 0.65f, 0.35f, 0.95f, 1.00f }; // Lighter Violet
    const ImVec4 Green       = { 0.20f, 0.80f, 0.40f, 1.00f };
    const ImVec4 GreenBg     = { 0.05f, 0.20f, 0.10f, 1.00f };
    const ImVec4 GreenText   = { 0.40f, 0.90f, 0.50f, 1.00f };
    const ImVec4 Yellow      = { 0.95f, 0.85f, 0.25f, 1.00f };
    const ImVec4 YellowBg    = { 0.25f, 0.22f, 0.05f, 1.00f };
    const ImVec4 Red         = { 0.95f, 0.25f, 0.25f, 1.00f };
    const ImVec4 RedBg       = { 0.25f, 0.05f, 0.05f, 1.00f };
    const ImVec4 RedBtn      = { 0.40f, 0.10f, 0.10f, 1.00f };
    const ImVec4 RedBtnH     = { 0.60f, 0.15f, 0.15f, 1.00f };
    const ImVec4 RedBtnA     = { 0.80f, 0.20f, 0.20f, 1.00f };
    const ImVec4 Muted       = { 0.70f, 0.70f, 0.70f, 1.00f };
    const ImVec4 Dim         = { 0.50f, 0.50f, 0.50f, 1.00f };
    const ImVec4 White       = { 1.00f, 1.00f, 1.00f, 1.00f };
    const ImVec4 Panel       = { 0.03f, 0.03f, 0.03f, 1.00f };
    const ImVec4 PanelSoft   = { 0.06f, 0.06f, 0.06f, 1.00f };
    const ImVec4 Border      = { 0.15f, 0.15f, 0.15f, 1.00f };
    const ImVec4 TableHead   = { 0.08f, 0.08f, 0.08f, 1.00f };
}

void SectionHeader(const char* label) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 5.0f));
    ImGui::TextColored(Colors::Accent, "%s", label);
    ImGui::PopStyleVar();
    ImGui::PushStyleColor(ImGuiCol_Separator, Colors::AccentSoft);
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void DrawMetricPill(const char* label, const std::string& value, const ImVec4& accent) {
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

void DrawStatusRow(const char* label, bool active, const char* onText, const char* offText) {
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

std::string g_UpdateVersion = "";

void DrawTopBar(DWORD pid, bool injected, bool databaseReady, bool liveDataReady) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::BeginChild("##topbar", ImVec2(0, 48), true, flags);
    ImGui::TextColored(Colors::Accent, "zigisoftware");
    ImGui::SameLine();
    ImGui::TextColored(Colors::Dim, "/");
    ImGui::SameLine();
    ImGui::TextColored(Colors::White, "LC Manager");
    ImGui::SameLine();
    ImGui::TextColored(Colors::Muted, LC_MANAGER_VERSION);

    bool isNewVersion = !g_UpdateVersion.empty() && 
                        (g_UpdateVersion[0] == 'v' || std::isdigit(g_UpdateVersion[0])) && 
                        g_UpdateVersion.find("error") == std::string::npos &&
                        g_UpdateVersion.find("Exception") == std::string::npos &&
                        g_UpdateVersion.find("Invoke-RestMethod") == std::string::npos &&
                        g_UpdateVersion != LC_MANAGER_VERSION && 
                        g_UpdateVersion != "v" LC_MANAGER_VERSION &&
                        std::string("v") + g_UpdateVersion != LC_MANAGER_VERSION;

    if (isNewVersion) {
        ImGui::SameLine();
        ImGui::TextColored(Colors::Red, " [Update Available: %s!]", g_UpdateVersion.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Go to Settings to download!");
        }
    }

    float right = ImGui::GetContentRegionAvail().x - 120.0f;
    if (right > 20.0f) ImGui::SameLine(ImGui::GetCursorPosX() + right);
    else ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, Colors::Red);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("FORCE CLOSE", ImVec2(100.0f, 24.0f))) {
        ShellExecuteA(NULL, "open", "cmd.exe", "/c taskkill /F /IM LobotomyCorp.exe /T", NULL, SW_HIDE);
    }
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::Spacing();
}

void DrawMetric(const char* label, const std::string& value) {
    ImGui::TextColored(Colors::Muted, "%s", label);
    ImGui::SameLine(180.0f);
    ImGui::TextColored(Colors::White, "%s", value.c_str());
}

void DrawNoLiveData(const std::string& error) {
    ImGui::BeginChild("##no_live_data", ImVec2(0, 0), true);
    SectionHeader("PIPELINE");
    ImGui::TextColored(Colors::Muted, "Live game snapshot is not ready yet.");
    ImGui::Spacing();
    ImGui::TextColored(Colors::Muted, "Reason: %s", error.empty() ? "unknown" : error.c_str());
    ImGui::Spacing();
    ImGui::TextColored(Colors::Muted, "Start the game, inject the module, then wait for the next data tick.");
    ImGui::EndChild();
}
