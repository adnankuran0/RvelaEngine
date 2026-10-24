#include "ToolBar.h"
#include "ImGui/imgui.h"
#include "Core/Engine.h"
#include "EditorSelection.h"
#include "Commands/CommandHistory.h"
#include "PrefabEditManager.h"
#include "Render/IconLibrary.h"
#include <cmath>

using namespace rv;

void ToolBar::Draw(Engine& engine)
{
    Scene& scene = engine.GetActiveScene();
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGuiWindowFlags toolbar_flags = ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoScrollbar;

    float menu_bar_height = ImGui::GetFrameHeight();
    ImVec2 toolbar_pos = ImVec2(viewport->Pos.x, viewport->Pos.y + menu_bar_height);
    ImVec2 toolbar_size = ImVec2(viewport->Size.x, 38.0f);

    ImGui::SetNextWindowPos(toolbar_pos);
    ImGui::SetNextWindowSize(toolbar_size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("ToolBar", nullptr, toolbar_flags);
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddLine(
            ImVec2(toolbar_pos.x, toolbar_pos.y + toolbar_size.y - 1.0f),
            ImVec2(toolbar_pos.x + toolbar_size.x, toolbar_pos.y + toolbar_size.y - 1.0f),
            ImGui::GetColorU32(ImGuiCol_Border)
        );

        ImVec2 button_size(44.0f, 26.0f);
        ImVec2 icon_size(18.0f, 18.0f);
        float spacing = 4.0f;
        float pill_padding_h = 4.0f;
        float pill_padding_v = 3.0f;

        float buttons_total_width = (3.0f * button_size.x) + (2.0f * spacing);
        float pill_width = buttons_total_width + (pill_padding_h * 2.0f);
        float pill_height = button_size.y + (pill_padding_v * 2.0f);

        float pill_x = std::floor((toolbar_size.x - pill_width) * 0.5f);
        float pill_y = std::floor((toolbar_size.y - pill_height) * 0.5f);

        ImVec2 pillMin = ImVec2(toolbar_pos.x + pill_x, toolbar_pos.y + pill_y);
        ImVec2 pillMax = ImVec2(pillMin.x + pill_width, pillMin.y + pill_height);

        drawList->AddRectFilled(pillMin, pillMax, ImGui::GetColorU32(ImGuiCol_FrameBg), 6.0f);
        drawList->AddRect(pillMin, pillMax, ImGui::GetColorU32(ImGuiCol_Border), 6.0f, 0, 1.0f);

        ImGui::SetCursorPos(ImVec2(pill_x + pill_padding_h, pill_y + pill_padding_v));

        ImTextureID playIcon  = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Play).GetID();
        ImTextureID pauseIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Pause).GetID();
        ImTextureID stopIcon  = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Stop).GetID();

        float pad_x = std::floor((button_size.x - icon_size.x) * 0.5f);
        float pad_y = std::floor((button_size.y - icon_size.y) * 0.5f);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.0f));

        const ImVec4 tintWhite(1.0f, 1.0f, 1.0f, 1.0f);

        // Play Button
        bool isPlaying = (scene.GetState() == SceneState::PLAY);
        if (isPlaying)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.063f, 0.450f, 0.280f, 1.0f)); // Emerald glow
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.080f, 0.550f, 0.340f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.050f, 0.380f, 0.230f, 1.0f));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.35f, 0.20f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.06f, 0.45f, 0.28f, 0.8f));
        }

        bool inPrefabMode = PrefabEditManager::Get().IsInPrefabMode();
        if (inPrefabMode)
        {
            ImGui::BeginDisabled();
        }
        if (ImGui::ImageButton("##PlayBtn", playIcon, icon_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintWhite))
        {
            engine.GetSceneManager().PlayScene();
        }
        if (inPrefabMode)
        {
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Pause Button
        bool isPaused = (scene.GetState() == SceneState::PAUSE);
        if (isPaused)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.38f, 0.05f, 1.0f)); // Amber glow
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.46f, 0.08f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.48f, 0.32f, 0.04f, 1.0f));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.30f, 0.05f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.38f, 0.05f, 0.8f));
        }

        if (ImGui::ImageButton("##PauseBtn", pauseIcon, icon_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintWhite))
        {
            engine.GetSceneManager().PauseScene();
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(isPaused ? "Paused" : "Pause Scene (F6)");
        }

        ImGui::SameLine();

        // Stop Button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.15f, 0.15f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.20f, 0.20f, 0.8f));

        if (ImGui::ImageButton("##StopBtn", stopIcon, icon_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintWhite))
        {
            engine.GetSceneManager().StopScene();
            EditorSelection::Get().Clear();
            CommandHistory::Get().SetCurrentScene(&engine.GetActiveScene());
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Stop Scene (F7)");
        }

        ImGui::PopStyleVar(3);
    }
    ImGui::End();
    ImGui::PopStyleVar(3);

    float dockspace_y = toolbar_pos.y + toolbar_size.y;
    float dockspace_height = viewport->Size.y - menu_bar_height - toolbar_size.y;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, dockspace_y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, dockspace_height));
    ImGui::SetNextWindowViewport(viewport->ID);
}
