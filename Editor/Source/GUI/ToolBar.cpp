#include "ToolBar.h"
#include "ImGui/imgui.h"
#include "Core/Engine.h"
#include "EditorUtils.h"

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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("ToolBar", nullptr, toolbar_flags);
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        ImVec2 button_size(80, 30);
        float total_buttons_width = 3 * button_size.x + 2 * ImGui::GetStyle().ItemSpacing.x;

        float offset = (toolbar_size.x - total_buttons_width) * 0.5f;
        if (offset > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
        }

        if (ImGui::Button("Play", button_size)) 
        {
            if((scene.GetState() == SceneState::EDIT && EditorUtils::SaveScene(engine)) || scene.GetState() == SceneState::PAUSE)
                scene.SetState(SceneState::PLAY);

        }
        ImGui::SameLine();

        if (ImGui::Button("Pause", button_size)) 
        {
            if (scene.GetState() == SceneState::PLAY)
                scene.SetState(SceneState::PAUSE);
        }
        ImGui::SameLine();

        if (ImGui::Button("Stop", button_size)) 
        {
            if (scene.GetState() == SceneState::PLAY || scene.GetState() == SceneState::PAUSE)
            {
                const std::string& scenePath = scene.GetPath();

                engine.GetSceneManager().LoadScene(scenePath);
                scene.SetState(SceneState::EDIT);
            }
        }

        ImGui::PopStyleColor();
    }
    ImGui::End();
    ImGui::PopStyleVar(3);

    float dockspace_y = toolbar_pos.y + toolbar_size.y;
    float dockspace_height = viewport->Size.y - menu_bar_height - toolbar_size.y;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, dockspace_y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, dockspace_height));
    ImGui::SetNextWindowViewport(viewport->ID);
}

