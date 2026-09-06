#include "ToolBar.h"
#include "ImGui/imgui.h"
#include "Core/Engine.h"
#include "EditorUtils.h"
#include "Render/IconLibrary.h"

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
    ImVec2 toolbar_size = ImVec2(viewport->Size.x, 40.0f);

    ImGui::SetNextWindowPos(toolbar_pos);
    ImGui::SetNextWindowSize(toolbar_size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("ToolBar", nullptr, toolbar_flags);
    {
        ImVec2 button_size(56.0f, 28.0f);
        ImVec2 icon_size(28.0f, 28.0f);

        float spacing = 6.0f;
        float total_width = (3.0f * button_size.x) + (2.0f * spacing);

        float offset_x = std::floor((toolbar_size.x - total_width) * 0.5f);
        float offset_y = std::floor((toolbar_size.y - button_size.y) * 0.5f);

        ImGui::SetCursorPos(ImVec2(offset_x, offset_y));

        ImTextureID playIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Play).GetID();
        ImTextureID pauseIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Pause).GetID();
        ImTextureID stopIcon = (ImTextureID)(intptr_t)IconLibrary::Get().GetIcon(EditorIcon::Stop).GetID();

        float pad_x = std::floor((button_size.x - icon_size.x) * 0.5f);
        float pad_y = std::floor((button_size.y - icon_size.y) * 0.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.0f));

        const ImVec4 tintWhite = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        bool isPlaying = (scene.GetState() == SceneState::PLAY);
        ImGui::PushStyleColor(ImGuiCol_Button, isPlaying ? ImVec4(0.20f, 0.35f, 0.20f, 1.0f) : ImVec4(0.16f, 0.16f, 0.22f, 1.0f));
        if (ImGui::ImageButton("##PlayBtn", playIcon, icon_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintWhite))
        {
            if ((scene.GetState() == SceneState::EDIT && EditorUtils::SaveScene(engine)) || scene.GetState() == SceneState::PAUSE)
            {
                scene.SetState(SceneState::PLAY);
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        bool isPaused = (scene.GetState() == SceneState::PAUSE);
        ImGui::PushStyleColor(ImGuiCol_Button, isPaused ? ImVec4(0.35f, 0.30f, 0.15f, 1.0f) : ImVec4(0.16f, 0.16f, 0.22f, 1.0f));
        if (ImGui::ImageButton("##PauseBtn", pauseIcon, icon_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintWhite))
        {
            if (scene.GetState() == SceneState::PLAY)
                scene.SetState(SceneState::PAUSE);
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.16f, 0.22f, 1.0f));
        if (ImGui::ImageButton("##StopBtn", stopIcon, icon_size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tintWhite))
        {
            if (scene.GetState() == SceneState::PLAY || scene.GetState() == SceneState::PAUSE)
            {
                const std::string& scenePath = scene.GetPath();
                engine.GetSceneManager().LoadScene(scenePath);
                scene.SetState(SceneState::EDIT);
            }
        }
        ImGui::PopStyleColor();

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