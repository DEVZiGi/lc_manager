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
#include <thread>
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
#include "style.h"
#include "utils.h"
#include "config.h"
#include "injector.h"
#include "ui_components.h"
#include "ui_tabs.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

void CheckForUpdatesThread() {
    std::string psCmd = "powershell -NoProfile -Command \"(Invoke-RestMethod -Uri 'https://api.github.com/repos/DEVZiGi/lc_manager/releases/latest').tag_name\"";
    std::string version = ExecHiddenCmd(psCmd);
    version.erase(version.find_last_not_of(" \n\r\t") + 1);
    version.erase(0, version.find_first_not_of(" \n\r\t"));
    g_UpdateVersion = version;
}

int main() {
    CoInitialize(NULL);

    std::thread updaterThread(CheckForUpdatesThread);
    updaterThread.detach();

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
    SaveConfig(settings, configPath); // Ensure commands.json is created with targetDir immediately

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

    ApplyPremiumStyle();

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
    
    double      sessionPlayTime = 0.0;
    bool        wasGameRunning  = false;
    double      lastSaveTime    = 0.0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        double dt = ImGui::GetIO().DeltaTime;

        if (frameCounter % 60 == 0) {
            lcProcessId = GetProcessIdByName("LobotomyCorp.exe");
            if (lcProcessId == 0) isInjected = false;
        }

        bool isGameRunning = (lcProcessId > 0);
        if (isGameRunning) {
            sessionPlayTime += dt;
            settings.totalPlayTime += dt;
            lastSaveTime += dt;
            if (lastSaveTime > 60.0) {
                SaveConfig(settings, configPath);
                lastSaveTime = 0.0;
            }
        } else {
            if (wasGameRunning) {
                sessionPlayTime = 0.0;
                SaveConfig(settings, configPath);
            }
        }
        wasGameRunning = isGameRunning;

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
            if (InjectMono(lcProcessId)) {
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

        
        static int selectedTab = 0;
        const char* tabNames[] = { "OVERVIEW", "AGENTS", "INVENTORY", "TODAY'S ORDEALS", "EXP TRACKER", "DETAILED INFO", "EXTRACTION", "INFO", "SETTINGS" };
        
        ImGui::Columns(2, "MainColumns", false);
        ImGui::SetColumnWidth(0, 200.0f);
        
        ImGui::BeginChild("##Sidebar", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
        ImGui::TextColored(ImVec4(0.22f, 0.74f, 0.98f, 1.0f), "LC MANAGER");
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.60f, 1.0f), LC_MANAGER_VERSION);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.1f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        for (int i = 0; i < 9; ++i) {
            bool isSelected = (selectedTab == i);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,1));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            }
            if (ImGui::Button(tabNames[i], ImVec2(-1, 35))) {
                selectedTab = i;
            }
            ImGui::PopStyleColor(2);
            ImGui::Spacing();
        }
        ImGui::PopStyleVar(2);
        
        // Bottom Sidebar Items
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 60.0f);
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::EndChild();
        ImGui::NextColumn();
        
        ImGui::BeginChild("##Content", ImVec2(0, 0), false);



            if (selectedTab == 0) {
                float panelW = ImGui::GetContentRegionAvail().x;
                float halfW  = panelW * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;

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

                ImGui::BeginChild("##ops", ImVec2(panelW, 120), true, staticFlags);
                SectionHeader("OPERATIONS");

                if (lcProcessId > 0 && !isInjected) {
                    if (ImGui::Button("INJECT MODULE", ImVec2(200, 38))) {
                        if (InjectMono(lcProcessId)) {
                            isInjected = true;
                        }
                    }
                } else if (isInjected) {
                    ImGui::Spacing();
                    ImGui::TextColored(Colors::Green, "  >>  PIPELINE OPEN  |  DATA SYNCING  <<");
                } else {
                    ImGui::Spacing();
                    if (ImGui::Button("LAUNCH GAME", ImVec2(200, 38))) {
                        ShellExecuteA(NULL, "open", "steam://rungameid/568220", NULL, NULL, SW_SHOWNORMAL);
                    }
                }
                ImGui::EndChild();
                ImGui::Spacing();

                ImGui::BeginChild("##prefs", ImVec2(panelW, 320), true, staticFlags);
                SectionHeader("PREFERENCES & GAME SPEED");

                bool prefsChanged = false;
                if (ImGui::Checkbox("Auto-inject on detection", &settings.autoInject)) {
                    prefsChanged = true;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Checkbox("Enable Turbo Modifiers", &settings.superFastForward)) {
                    prefsChanged = true;
                }
                
                if (settings.superFastForward) {
                    ImGui::Indent();
                    ImGui::TextColored(Colors::Muted, "These sliders override the in-game x1.5 and x2 speed settings.");
                    if (ImGui::SliderFloat("Speed x1.5 Override", &settings.tt15Speed, 5.0f, 20.0f, "%.1f")) {
                        prefsChanged = true;
                    }
                    if (ImGui::SliderFloat("Speed x2 Override", &settings.tt2Speed, 5.0f, 20.0f, "%.1f")) {
                        prefsChanged = true;
                    }
                    ImGui::Unindent();
                }

                if (prefsChanged) {
                    SaveConfig(settings, configPath);
                }

                ImGui::EndChild();
                ImGui::Spacing();

                ImGui::BeginChild("##stats", ImVec2(panelW, 110), true, staticFlags);
                SectionHeader("STATISTICS");
                
                int totalH = (int)(settings.totalPlayTime / 3600);
                int totalM = (int)(fmod(settings.totalPlayTime, 3600) / 60);
                int totalS = (int)fmod(settings.totalPlayTime, 60);
                ImGui::Text("Total App Time: %02d:%02d:%02d", totalH, totalM, totalS);

                int sessH = (int)(sessionPlayTime / 3600);
                int sessM = (int)(fmod(sessionPlayTime, 3600) / 60);
                int sessS = (int)fmod(sessionPlayTime, 60);
                ImGui::Text("Session Time: %02d:%02d:%02d", sessH, sessM, sessS);

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
                }

            bool pipelineReady = (isInjected && lcProcessId > 0);
            if (pipelineReady) {
                if (selectedTab == 1) {
                    DrawAgentsTab(liveData, liveDataLoaded, liveDataError, dbManager);
                    }
                if (selectedTab == 2) {
                    DrawInventoryTab(liveData, liveDataLoaded, liveDataError, dbManager);
                    }
                if (selectedTab == 3) {
                    DrawOrdealTab(liveData, liveDataLoaded, liveDataError);
                    }
                if (selectedTab == 4) {
                    DrawExpTrackerTab(liveData, liveDataLoaded, liveDataError);
                    }
                if (selectedTab == 5) {
                    DrawDetailedInfoTab(liveData, liveDataLoaded, liveDataError, dbManager);
                    }
            }

            const json* extractChoices = FindAny(liveData, {"extractionChoices"});
            bool hasExtraction = extractChoices != nullptr && extractChoices->is_array() && extractChoices->size() > 0;

            bool highlight = hasExtraction && ((ImGui::GetTime() * 2.0f) - floor(ImGui::GetTime() * 2.0f) > 0.5f);
            if (highlight) ImGui::PushStyleColor(ImGuiCol_Text, Colors::Green);
            if (selectedTab == 6) {
                if (highlight) ImGui::PopStyleColor();
                DrawExtractionTab(liveData, dbManager);
                } else {
                if (highlight) ImGui::PopStyleColor();
            }

            if (selectedTab == 7) {
                DrawInfoTab();
            }

            if (selectedTab == 8) {
                DrawSettingsTab(settings, configPath);
            }

            ImGui::EndChild();
        ImGui::Columns(1);

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