#include "Style.h"
#include <imgui.h>
#include "Utils/FileUtils.h"

void rv::SetStyle()
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(
        EDITOR_PATH("Fonts\\roboto.ttf").GetAbsoluteStr().c_str(), 16.0f
    );

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    ImVec4 base = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
    ImVec4 mantle = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    ImVec4 crust = ImVec4(0.06f, 0.06f, 0.09f, 1.00f);

    ImVec4 surface0 = ImVec4(0.17f, 0.17f, 0.23f, 1.00f);
    ImVec4 surface1 = ImVec4(0.20f, 0.20f, 0.27f, 1.00f);

    ImVec4 accent = ImVec4(0.62f, 0.56f, 0.80f, 1.00f);
    ImVec4 accentHover = ImVec4(0.55f, 0.50f, 0.72f, 1.00f);
    ImVec4 accentActive = ImVec4(0.68f, 0.62f, 0.88f, 1.00f);

    colors[ImGuiCol_Text] = ImVec4(0.88f, 0.89f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.57f, 0.70f, 1.00f);

    colors[ImGuiCol_WindowBg] = base;
    colors[ImGuiCol_ChildBg] = base;
    colors[ImGuiCol_PopupBg] = mantle;
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.45f, 0.40f);

    colors[ImGuiCol_FrameBg] = surface0;
    colors[ImGuiCol_FrameBgHovered] = surface1;
    colors[ImGuiCol_FrameBgActive] = surface1;

    colors[ImGuiCol_TitleBg] = mantle;
    colors[ImGuiCol_TitleBgActive] = surface0;
    colors[ImGuiCol_TitleBgCollapsed] = mantle;

    colors[ImGuiCol_Button] = surface0;
    colors[ImGuiCol_ButtonHovered] = accentHover;
    colors[ImGuiCol_ButtonActive] = accentActive;

    colors[ImGuiCol_Header] = surface0;
    colors[ImGuiCol_HeaderHovered] = accentHover;
    colors[ImGuiCol_HeaderActive] = accent;


    colors[ImGuiCol_Tab] = mantle;
    colors[ImGuiCol_TabHovered] = accentHover;
    colors[ImGuiCol_TabActive] = surface0;
    colors[ImGuiCol_TabUnfocused] = crust;
    colors[ImGuiCol_TabUnfocusedActive] = surface0;

    colors[ImGuiCol_ScrollbarBg] = crust;
    colors[ImGuiCol_ScrollbarGrab] = surface0;
    colors[ImGuiCol_ScrollbarGrabHovered] = accentHover;
    colors[ImGuiCol_ScrollbarGrabActive] = accentActive;

    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accentActive;

    colors[ImGuiCol_Separator] = surface1;
    colors[ImGuiCol_SeparatorHovered] = accentHover;
    colors[ImGuiCol_SeparatorActive] = accentActive;

    colors[ImGuiCol_ResizeGrip] = surface1;
    colors[ImGuiCol_ResizeGripHovered] = accentHover;
    colors[ImGuiCol_ResizeGripActive] = accentActive;

    colors[ImGuiCol_TextSelectedBg] = accentHover;
    colors[ImGuiCol_NavHighlight] = accent;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.50f);

    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;

    style.FramePadding = ImVec2(4, 4);
    style.ItemSpacing = ImVec2(10, 6);
    style.WindowPadding = ImVec2(12, 8);
}

