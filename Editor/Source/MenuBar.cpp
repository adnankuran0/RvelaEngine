#include "MenuBar.h"
#include <ImGui/tinyfiledialogs.h>
#include <fstream>

void MenuBar::Draw(Engine* engine)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New scene")) 
            {

            }
            if (ImGui::MenuItem("Open scene")) 
            {
                const char* filterPatterns[] = { "*.rscene" };
                const char* filePath = tinyfd_openFileDialog("Select a scene", "", 1, filterPatterns, NULL, 0);

                std::string file = filePath ? std::string(filePath) : "";
                if (!file.empty())
                {
                    engine->GetSceneManager()->LoadScene(*engine->GetScene(), file);
                }
                
            }
            if (ImGui::MenuItem("Save scene"))
            {
                const char* filterPatterns[] = { "*.rscene" };
                const char* filePath = tinyfd_saveFileDialog("Save Scene As", "scene.rscene", 1, filterPatterns, NULL);

                std::string file = filePath ? std::string(filePath) : "";
                std::cout << "Scene tried to save at " << file << "\n";
                if (!file.empty())
                {
                    std::ofstream ofs(file);
                    if (ofs.is_open())
                    {
                        ofs.close();
                        engine->GetSceneManager()->SaveScene(*engine->GetScene(), file);
                    }
                }
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