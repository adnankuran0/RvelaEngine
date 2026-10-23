#pragma once

#include <imgui.h>

namespace rv {

enum class EditorTheme
{
    DeepSlate = 0,
    TokyoNight,
    Catppuccin,
    ObsidianCarbon,
    WarmCharcoal
};

void SetStyle(EditorTheme theme = EditorTheme::DeepSlate);
void ApplyTheme(EditorTheme theme);
EditorTheme GetCurrentTheme();
const char* GetThemeName(EditorTheme theme);
int GetThemeCount();

ImVec4 GetAccentColor();
ImVec4 GetAccentHoverColor();
ImVec4 GetAccentActiveColor();
ImVec4 GetMutedTextColor();
ImVec4 GetSuccessColor();
ImVec4 GetWarningColor();
ImVec4 GetErrorColor();

}
