#include "Style.h"
#include <imgui.h>
#include "Utils/FileUtils.h"

namespace rv {

static EditorTheme s_CurrentTheme = EditorTheme::DeepSlate;
static ImVec4 s_AccentColor = ImVec4(0.298f, 0.431f, 0.961f, 1.00f);
static ImVec4 s_AccentHoverColor = ImVec4(0.361f, 0.486f, 0.980f, 1.00f);
static ImVec4 s_AccentActiveColor = ImVec4(0.231f, 0.357f, 0.859f, 1.00f);
static ImVec4 s_MutedTextColor = ImVec4(0.550f, 0.580f, 0.680f, 1.00f);
static const ImVec4 s_SuccessColor = ImVec4(0.063f, 0.725f, 0.506f, 1.00f); // #10b981
static const ImVec4 s_WarningColor = ImVec4(0.961f, 0.620f, 0.043f, 1.00f); // #f59e0b
static const ImVec4 s_ErrorColor = ImVec4(0.937f, 0.267f, 0.267f, 1.00f);   // #ef4444
static ImVec4 s_DarkTextColor = ImVec4(0.067f, 0.067f, 0.106f, 1.00f);

void InitFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts->Fonts.empty())
    {
        static const ImWchar turkishRanges[] = {
            0x0020, 0x017F,
            0
        };
        io.Fonts->AddFontFromFileTTF(
            EDITOR_PATH("Fonts\\roboto.ttf").GetAbsoluteStr().c_str(), 16.0f, nullptr, turkishRanges
        );
    }
}

void ApplyTheme(EditorTheme theme)
{
    s_CurrentTheme = theme;
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    ImVec4 base, mantle, crust, surface0, surface1, surface2;
    ImVec4 accent, accentHover, accentActive;
    ImVec4 text, textDisabled, textMuted;
    ImVec4 border, borderSubtle;

    switch (theme)
    {
    case EditorTheme::TokyoNight:
    {
        base         = ImVec4(0.102f, 0.106f, 0.149f, 1.00f); // #1a1b26
        mantle       = ImVec4(0.086f, 0.086f, 0.118f, 1.00f); // #16161e
        crust        = ImVec4(0.075f, 0.075f, 0.102f, 1.00f); // #13131a

        surface0     = ImVec4(0.141f, 0.157f, 0.231f, 1.00f); // #24283b
        surface1     = ImVec4(0.184f, 0.208f, 0.310f, 1.00f); // #2f354f
        surface2     = ImVec4(0.247f, 0.278f, 0.400f, 1.00f);

        accent       = ImVec4(0.478f, 0.635f, 0.969f, 1.00f); // #7aa2f7
        accentHover  = ImVec4(0.576f, 0.722f, 0.992f, 1.00f);
        accentActive = ImVec4(0.733f, 0.604f, 0.969f, 1.00f); // #bb9af7

        text         = ImVec4(0.753f, 0.792f, 0.961f, 1.00f); // #c0caf5
        textDisabled = ImVec4(0.350f, 0.380f, 0.520f, 1.00f);
        textMuted    = ImVec4(0.470f, 0.490f, 0.600f, 1.00f);

        border       = ImVec4(0.240f, 0.280f, 0.400f, 0.50f);
        borderSubtle = ImVec4(0.184f, 0.208f, 0.310f, 0.40f);
        break;
    }
    case EditorTheme::Catppuccin:
    {
        base         = ImVec4(0.118f, 0.118f, 0.180f, 1.00f); // #1e1e2e (Base)
        mantle       = ImVec4(0.094f, 0.094f, 0.145f, 1.00f); // #181825 (Mantle)
        crust        = ImVec4(0.067f, 0.067f, 0.106f, 1.00f); // #11111b (Crust)

        surface0     = ImVec4(0.192f, 0.196f, 0.267f, 1.00f); // #313244 (Surface0)
        surface1     = ImVec4(0.271f, 0.278f, 0.353f, 1.00f); // #45475a (Surface1)
        surface2     = ImVec4(0.345f, 0.357f, 0.439f, 1.00f); // #585b70 (Surface2)

        accent       = ImVec4(0.796f, 0.651f, 0.969f, 1.00f); // #cba6f7 (Mauve)
        accentHover  = ImVec4(0.961f, 0.761f, 0.906f, 1.00f); // #f5c2e7 (Pink)
        accentActive = ImVec4(0.706f, 0.745f, 0.996f, 1.00f); // #b4befe (Lavender)

        text         = ImVec4(0.804f, 0.839f, 0.957f, 1.00f); // #cdd6f4 (Text)
        textDisabled = ImVec4(0.424f, 0.439f, 0.525f, 1.00f); // #6c7086 (Overlay0)
        textMuted    = ImVec4(0.651f, 0.678f, 0.784f, 1.00f); // #a6adc8 (Subtext0)

        border       = ImVec4(0.271f, 0.278f, 0.353f, 0.65f); // Surface1
        borderSubtle = ImVec4(0.192f, 0.196f, 0.267f, 0.50f); // Surface0
        break;
    }
    case EditorTheme::ObsidianCarbon:
    {
        base         = ImVec4(0.071f, 0.071f, 0.078f, 1.00f); // #121214
        mantle       = ImVec4(0.051f, 0.051f, 0.059f, 1.00f); // #0d0d0f
        crust        = ImVec4(0.035f, 0.035f, 0.043f, 1.00f); // #09090b

        surface0     = ImVec4(0.118f, 0.118f, 0.141f, 1.00f); // #1e1e24
        surface1     = ImVec4(0.165f, 0.165f, 0.196f, 1.00f); // #2a2a32
        surface2     = ImVec4(0.220f, 0.220f, 0.260f, 1.00f);

        accent       = ImVec4(0.024f, 0.714f, 0.831f, 1.00f); // #06b6d4 (cyan)
        accentHover  = ImVec4(0.133f, 0.827f, 0.933f, 1.00f); // #22d3ee
        accentActive = ImVec4(0.059f, 0.584f, 0.690f, 1.00f);

        text         = ImVec4(0.957f, 0.957f, 0.961f, 1.00f); // #f4f4f5
        textDisabled = ImVec4(0.443f, 0.443f, 0.478f, 1.00f); // #71717a
        textMuted    = ImVec4(0.550f, 0.550f, 0.600f, 1.00f);

        border       = ImVec4(0.200f, 0.200f, 0.240f, 0.60f);
        borderSubtle = ImVec4(0.140f, 0.140f, 0.170f, 0.50f);
        break;
    }
    case EditorTheme::WarmCharcoal:
    {
        base         = ImVec4(0.110f, 0.106f, 0.102f, 1.00f); // #1c1b1a
        mantle       = ImVec4(0.082f, 0.078f, 0.075f, 1.00f); // #151413
        crust        = ImVec4(0.063f, 0.059f, 0.055f, 1.00f); // #100f0e

        surface0     = ImVec4(0.157f, 0.149f, 0.141f, 1.00f); // #282624
        surface1     = ImVec4(0.204f, 0.196f, 0.188f, 1.00f); // #343230
        surface2     = ImVec4(0.270f, 0.259f, 0.247f, 1.00f);

        accent       = ImVec4(0.961f, 0.620f, 0.043f, 1.00f); // #f59e0b (amber)
        accentHover  = ImVec4(0.984f, 0.749f, 0.141f, 1.00f); // #fbbf24
        accentActive = ImVec4(0.851f, 0.467f, 0.024f, 1.00f);

        text         = ImVec4(0.961f, 0.961f, 0.957f, 1.00f); // #f5f5f4
        textDisabled = ImVec4(0.471f, 0.443f, 0.424f, 1.00f); // #78716c
        textMuted    = ImVec4(0.580f, 0.550f, 0.530f, 1.00f);

        border       = ImVec4(0.250f, 0.230f, 0.220f, 0.55f);
        borderSubtle = ImVec4(0.180f, 0.170f, 0.160f, 0.45f);
        break;
    }
    case EditorTheme::DeepSlate:
    default:
    {
        base         = ImVec4(0.090f, 0.094f, 0.122f, 1.00f); // #17181f
        mantle       = ImVec4(0.075f, 0.078f, 0.098f, 1.00f); // #131419
        crust        = ImVec4(0.059f, 0.063f, 0.078f, 1.00f); // #0f1014

        surface0     = ImVec4(0.125f, 0.133f, 0.169f, 1.00f); // #20222b
        surface1     = ImVec4(0.165f, 0.176f, 0.227f, 1.00f); // #2a2d3a
        surface2     = ImVec4(0.220f, 0.235f, 0.302f, 1.00f);

        accent       = ImVec4(0.298f, 0.431f, 0.961f, 1.00f); // #4c6ef5 (indigo)
        accentHover  = ImVec4(0.361f, 0.486f, 0.980f, 1.00f); // #5c7cfa
        accentActive = ImVec4(0.231f, 0.357f, 0.859f, 1.00f); // #3b5bdb

        text         = ImVec4(0.922f, 0.933f, 0.961f, 1.00f); // #ebedf5
        textDisabled = ImVec4(0.388f, 0.404f, 0.478f, 1.00f); // #63677a
        textMuted    = ImVec4(0.550f, 0.580f, 0.680f, 1.00f);

        border       = ImVec4(0.200f, 0.220f, 0.290f, 0.55f);
        borderSubtle = ImVec4(0.140f, 0.150f, 0.200f, 0.50f);
        break;
    }
    }

    s_AccentColor = accent;
    s_AccentHoverColor = accentHover;
    s_AccentActiveColor = accentActive;
    s_MutedTextColor = textMuted;
    s_DarkTextColor = crust;

    colors[ImGuiCol_Text]                  = text;
    colors[ImGuiCol_TextDisabled]          = textDisabled;

    colors[ImGuiCol_WindowBg]              = base;
    colors[ImGuiCol_ChildBg]               = mantle;
    colors[ImGuiCol_PopupBg]               = mantle;
    colors[ImGuiCol_Border]                = border;
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    colors[ImGuiCol_FrameBg]               = surface0;
    colors[ImGuiCol_FrameBgHovered]        = surface1;
    colors[ImGuiCol_FrameBgActive]         = surface2;

    colors[ImGuiCol_TitleBg]               = crust;
    colors[ImGuiCol_TitleBgActive]         = mantle;
    colors[ImGuiCol_TitleBgCollapsed]      = crust;

    colors[ImGuiCol_MenuBarBg]             = crust;

    colors[ImGuiCol_ScrollbarBg]           = crust;
    colors[ImGuiCol_ScrollbarGrab]         = surface1;
    colors[ImGuiCol_ScrollbarGrabHovered]  = accentHover;
    colors[ImGuiCol_ScrollbarGrabActive]   = accentActive;

    colors[ImGuiCol_CheckMark]             = accent;
    colors[ImGuiCol_SliderGrab]            = accent;
    colors[ImGuiCol_SliderGrabActive]      = accentActive;

    colors[ImGuiCol_Button]                = surface0;
    colors[ImGuiCol_ButtonHovered]         = surface1;
    colors[ImGuiCol_ButtonActive]          = surface2;

    colors[ImGuiCol_Header]                = surface0;
    colors[ImGuiCol_HeaderHovered]         = surface1;
    colors[ImGuiCol_HeaderActive]          = surface2;

    colors[ImGuiCol_Separator]             = borderSubtle;
    colors[ImGuiCol_SeparatorHovered]      = accentHover;
    colors[ImGuiCol_SeparatorActive]       = accentActive;

    colors[ImGuiCol_ResizeGrip]            = surface0;
    colors[ImGuiCol_ResizeGripHovered]     = accentHover;
    colors[ImGuiCol_ResizeGripActive]      = accentActive;

    colors[ImGuiCol_Tab]                   = crust;
    colors[ImGuiCol_TabHovered]            = surface0;
    colors[ImGuiCol_TabActive]             = base;
    colors[ImGuiCol_TabUnfocused]          = crust;
    colors[ImGuiCol_TabUnfocusedActive]    = mantle;

    colors[ImGuiCol_DockingPreview]        = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_DockingEmptyBg]        = crust;

    colors[ImGuiCol_PlotLines]             = accent;
    colors[ImGuiCol_PlotLinesHovered]      = accentHover;
    colors[ImGuiCol_PlotHistogram]         = accent;
    colors[ImGuiCol_PlotHistogramHovered]  = accentHover;

    colors[ImGuiCol_TableHeaderBg]         = mantle;
    colors[ImGuiCol_TableBorderStrong]     = border;
    colors[ImGuiCol_TableBorderLight]      = borderSubtle;
    colors[ImGuiCol_TableRowBg]            = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);

    colors[ImGuiCol_TextSelectedBg]        = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_DragDropTarget]        = accent;
    colors[ImGuiCol_NavHighlight]          = accent;
    colors[ImGuiCol_NavWindowingHighlight] = accent;
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.60f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.0f, 0.0f, 0.0f, 0.65f);
}

void SetStyle(EditorTheme theme)
{
    InitFonts();

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 5.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 5.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;

    style.WindowPadding     = ImVec2(10.0f, 10.0f);
    style.FramePadding      = ImVec2(8.0f, 5.0f);
    style.CellPadding       = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
    style.IndentSpacing     = 18.0f;

    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 10.0f;
    style.DockingSeparatorSize = 2.0f;

    ApplyTheme(theme);
}

EditorTheme GetCurrentTheme()
{
    return s_CurrentTheme;
}

const char* GetThemeName(EditorTheme theme)
{
    switch (theme)
    {
    case EditorTheme::TokyoNight:     return "Tokyo Night (Violet)";
    case EditorTheme::Catppuccin:     return "Catppuccin (Mocha)";
    case EditorTheme::ObsidianCarbon: return "Obsidian (Carbon)";
    case EditorTheme::WarmCharcoal:   return "Warm Charcoal (Amber)";
    case EditorTheme::DeepSlate:
    default:                          return "Deep Slate (Midnight)";
    }
}

int GetThemeCount()
{
    return 5;
}

ImVec4 GetAccentColor()
{
    return s_AccentColor;
}

ImVec4 GetAccentHoverColor()
{
    return s_AccentHoverColor;
}

ImVec4 GetAccentActiveColor()
{
    return s_AccentActiveColor;
}

ImVec4 GetMutedTextColor()
{
    return s_MutedTextColor;
}

ImVec4 GetSuccessColor()
{
    return s_SuccessColor;
}

ImVec4 GetWarningColor()
{
    return s_WarningColor;
}

ImVec4 GetErrorColor()
{
    return s_ErrorColor;
}

ImVec4 GetDarkTextColor()
{
    return s_DarkTextColor;
}

bool IsAccentBright()
{
    float lum = 0.2126f * s_AccentColor.x + 0.7152f * s_AccentColor.y + 0.0722f * s_AccentColor.z;
    return lum > 0.45f;
}

ImVec4 GetSelectionTextColor()
{
    return IsAccentBright() ? s_DarkTextColor : ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

}
