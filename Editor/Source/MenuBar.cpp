#include "MenuBar.h"

void MenuBar::Draw(Engine* engine)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New scene")) {}
            if (ImGui::MenuItem("Open scene")) 
            {
                engine->GetSceneManager()->LoadScene(*engine->GetScene(), "D:\\GitHub\\RvelaEngine\\TestProject\\Scenes\\Test.rscene");
            }
            if (ImGui::MenuItem("Save scene")) 
            {
                engine->GetSceneManager()->SaveScene(*engine->GetScene(), "D:\\GitHub\\RvelaEngine\\TestProject\\Scenes\\Test.rscene");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) 
            {
                glfwSetWindowShouldClose(engine->GetWindow()->GetGLFWWindow(), true);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo")) {}
            if (ImGui::MenuItem("Redo")) {}
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}