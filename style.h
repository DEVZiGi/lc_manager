#pragma once
#include "imgui.h"

static void ApplyPremiumStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.54f, 0.17f, 0.89f, 0.50f); // Purple Border
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.06f, 0.06f, 0.06f, 1.00f); 
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.30f, 0.10f, 0.50f, 1.00f); // Hover Purple
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.54f, 0.17f, 0.89f, 1.00f); // Active Purple
    colors[ImGuiCol_TitleBg]                = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
    
    // Purple Accents
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.10f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.54f, 0.17f, 0.89f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.65f, 0.35f, 0.95f, 1.00f);
    
    // Texts - White
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    colors[ImGuiCol_CheckMark]              = ImVec4(0.54f, 0.17f, 0.89f, 1.00f); // Purple
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.54f, 0.17f, 0.89f, 1.00f); // Purple
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.65f, 0.35f, 0.95f, 1.00f);
    
    // Buttons
    colors[ImGuiCol_Button]                 = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.30f, 0.10f, 0.50f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.54f, 0.17f, 0.89f, 1.00f);
    
    colors[ImGuiCol_Header]                 = ImVec4(0.30f, 0.10f, 0.50f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.54f, 0.17f, 0.89f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.65f, 0.35f, 0.95f, 1.00f);
    
    colors[ImGuiCol_Separator]              = ImVec4(0.30f, 0.10f, 0.50f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.54f, 0.17f, 0.89f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.65f, 0.35f, 0.95f, 1.00f);
    
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.54f, 0.17f, 0.89f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.54f, 0.17f, 0.89f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.65f, 0.35f, 0.95f, 0.95f);

    colors[ImGuiCol_Tab]                    = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.30f, 0.10f, 0.50f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.54f, 0.17f, 0.89f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.30f, 0.10f, 0.50f, 1.00f);
    
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.54f, 0.17f, 0.89f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
    
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 0.0f;
    
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.TabBorderSize     = 1.0f;
    
    style.ItemSpacing       = ImVec2(10, 12);
    style.FramePadding      = ImVec2(10, 8);
    style.WindowPadding     = ImVec2(16, 16);
}
