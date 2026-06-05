#pragma once

#include <string>
#include <windows.h>
#include "imgui.h"

#define LC_MANAGER_VERSION "v0.2"

namespace Colors {
    extern const ImVec4 Accent;
    extern const ImVec4 AccentSoft;
    extern const ImVec4 Link;
    extern const ImVec4 Green;
    extern const ImVec4 GreenBg;
    extern const ImVec4 GreenText;
    extern const ImVec4 Yellow;
    extern const ImVec4 YellowBg;
    extern const ImVec4 Red;
    extern const ImVec4 RedBg;
    extern const ImVec4 RedBtn;
    extern const ImVec4 RedBtnH;
    extern const ImVec4 RedBtnA;
    extern const ImVec4 Muted;
    extern const ImVec4 Dim;
    extern const ImVec4 White;
    extern const ImVec4 Panel;
    extern const ImVec4 PanelSoft;
    extern const ImVec4 Border;
    extern const ImVec4 TableHead;
}

void SectionHeader(const char* label);
void DrawMetricPill(const char* label, const std::string& value, const ImVec4& accent = Colors::Accent);
void DrawStatusRow(const char* label, bool active, const char* onText, const char* offText = "OFFLINE");
void DrawTopBar(DWORD pid, bool injected, bool databaseReady, bool liveDataReady);
void DrawMetric(const char* label, const std::string& value);
void DrawNoLiveData(const std::string& error);

extern std::string g_UpdateVersion;
