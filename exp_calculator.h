#ifndef LC_MANAGER_EXP_CALCULATOR_H
#define LC_MANAGER_EXP_CALCULATOR_H

#endif

#pragma once
#include "imgui.h"
#include <algorithm>

namespace ExpCalculator {
    inline int StatTier(int value) {
        if (value >= 100) return 6;
        if (value >= 85) return 5;
        if (value >= 65) return 4;
        if (value >= 45) return 3;
        if (value >= 30) return 2;
        return 1;
    }

    inline const char* TierLabel(int tier) {
        switch (tier) {
            case 6: return "EX";
            case 5: return "V";
            case 4: return "IV";
            case 3: return "III";
            case 2: return "II";
            default: return "I";
        }
    }

    inline int OverallLevelFromStatTiers(int totalTierLevels) {
        if (totalTierLevels >= 16) return 5;
        if (totalTierLevels >= 12) return 4;
        if (totalTierLevels >= 9) return 3;
        if (totalTierLevels >= 6) return 2;
        return 1;
    }

    inline int GetNextStatThreshold(int currentValue) {
        if (currentValue < 30) return 30;
        if (currentValue < 45) return 45;
        if (currentValue < 65) return 65;
        if (currentValue < 85) return 85;
        if (currentValue < 100) return 100;
        return -1; // Maxed out
    }

    inline void DrawCalculatorTab() {
        static int stats[4] = {15, 15, 15, 15}; // F, P, T, J

        ImGui::BeginChild("##calculator_panel", ImVec2(0, 0), true);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 5.0f));
        ImGui::TextColored(ImVec4(0.98f, 0.73f, 0.01f, 1.0f), "AGENT LEVEL CALCULATOR");
        ImGui::PopStyleVar();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped("BLAH BLAH BLAH");
        ImGui::Spacing();

        ImGui::PushItemWidth(150.0f);
        ImGui::InputInt("Fortitude", &stats[0]);
        ImGui::InputInt("Prudence", &stats[1]);
        ImGui::InputInt("Temperance", &stats[2]);
        ImGui::InputInt("Justice", &stats[3]);
        ImGui::PopItemWidth();

        for (int i = 0; i < 4; ++i) {
            if (stats[i] < 0) stats[i] = 0;
            if (stats[i] > 130) stats[i] = 130;
        }

        int tiers[4] = {StatTier(stats[0]), StatTier(stats[1]), StatTier(stats[2]), StatTier(stats[3])};
        int totalTiers = tiers[0] + tiers[1] + tiers[2] + tiers[3];
        int overallLevel = OverallLevelFromStatTiers(totalTiers);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Current Overall Level: ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), "%s (Total Stat Tiers: %d)", TierLabel(overallLevel), totalTiers);

        int nextOverallThreshold = 0;
        if (totalTiers < 6) nextOverallThreshold = 6;
        else if (totalTiers < 9) nextOverallThreshold = 9;
        else if (totalTiers < 12) nextOverallThreshold = 12;
        else if (totalTiers < 16) nextOverallThreshold = 16;

        if (nextOverallThreshold > 0) {
            ImGui::TextColored(ImVec4(0.98f, 0.73f, 0.01f, 1.0f), "Tiers needed for next Overall Level: %d", nextOverallThreshold - totalTiers);
        } else {
            ImGui::TextColored(ImVec4(0.98f, 0.73f, 0.01f, 1.0f), "MAX OVERALL LEVEL ACHIEVED");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const char* statNames[4] = {"Fortitude (Instinct)", "Prudence (Insight)", "Temperance (Attachment)", "Justice (Repression)"};
        for (int i = 0; i < 4; ++i) {
            int nextVal = GetNextStatThreshold(stats[i]);
            if (nextVal > 0) {
                ImGui::Text("%s: Next tier (%s) at %d stats ", statNames[i], TierLabel(tiers[i] + 1), nextVal);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.95f, 0.25f, 0.25f, 1.0f), "(needs +%d points)", nextVal - stats[i]);
            } else {
                ImGui::Text("%s: ", statNames[i]);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), "MAX TIER (EX)");
            }
        }

        ImGui::EndChild();
    }
}
