#pragma once
#include "ImGui/imgui.h"

void SetStyle()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\RvelaEngine\\Resources\\Editor\\roboto.ttf", 16.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    ImVec4 bg = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    ImVec4 panel = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    ImVec4 lightBg = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    ImVec4 redAccent = ImVec4(0.60f, 0.10f, 0.20f, 1.00f);
    ImVec4 redAccentHovered = ImVec4(0.70f, 0.15f, 0.25f, 1.00f);
    ImVec4 redAccentActive = ImVec4(0.80f, 0.20f, 0.30f, 1.00f);

    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg] = bg;
    colors[ImGuiCol_ChildBg] = bg;
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    colors[ImGuiCol_FrameBg] = panel;
    colors[ImGuiCol_FrameBgHovered] = lightBg;
    colors[ImGuiCol_FrameBgActive] = redAccent;

    colors[ImGuiCol_TitleBg] = bg;
    colors[ImGuiCol_TitleBgActive] = panel;
    colors[ImGuiCol_TitleBgCollapsed] = bg;

    colors[ImGuiCol_MenuBarBg] = panel;
    colors[ImGuiCol_ScrollbarBg] = bg;
    colors[ImGuiCol_ScrollbarGrab] = lightBg;
    colors[ImGuiCol_ScrollbarGrabHovered] = redAccent;
    colors[ImGuiCol_ScrollbarGrabActive] = redAccentActive;

    colors[ImGuiCol_CheckMark] = redAccent;
    colors[ImGuiCol_SliderGrab] = redAccent;
    colors[ImGuiCol_SliderGrabActive] = redAccentActive;

    colors[ImGuiCol_Button] = redAccent;
    colors[ImGuiCol_ButtonHovered] = redAccentHovered;
    colors[ImGuiCol_ButtonActive] = redAccentActive;

    colors[ImGuiCol_Header] = redAccent;
    colors[ImGuiCol_HeaderHovered] = redAccentHovered;
    colors[ImGuiCol_HeaderActive] = redAccentActive;

    colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = redAccentHovered;
    colors[ImGuiCol_SeparatorActive] = redAccentActive;

    colors[ImGuiCol_ResizeGrip] = redAccent;
    colors[ImGuiCol_ResizeGripHovered] = redAccentHovered;
    colors[ImGuiCol_ResizeGripActive] = redAccentActive;

    colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered] = redAccentHovered;
    colors[ImGuiCol_TabActive] = redAccentActive;
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = redAccent;

    colors[ImGuiCol_PlotLines] = redAccent;
    colors[ImGuiCol_PlotLinesHovered] = redAccentHovered;
    colors[ImGuiCol_PlotHistogram] = redAccent;
    colors[ImGuiCol_PlotHistogramHovered] = redAccentHovered;

    colors[ImGuiCol_TextSelectedBg] = redAccent;
    colors[ImGuiCol_DragDropTarget] = redAccent;
    colors[ImGuiCol_NavHighlight] = redAccent;
    colors[ImGuiCol_NavWindowingHighlight] = redAccent;
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0, 0, 0, 0.30f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.50f);

    style.FrameRounding = 6.0f;     
    style.GrabRounding = 6.0f;      
    style.WindowRounding = 8.0f;    
    style.PopupRounding = 6.0f;     
    style.ChildRounding = 6.0f;    
    style.ScrollbarRounding = 6.0f;

    style.FramePadding = ImVec2(8, 4); 
    style.ItemSpacing = ImVec2(10, 6); 
    style.IndentSpacing = 20.0f;       
    style.WindowPadding = ImVec2(12, 8);
}
