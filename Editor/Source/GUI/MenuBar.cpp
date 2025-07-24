#include "MenuBar.h"
#include <ImGui/tinyfiledialogs.h>
#include <fstream>

void MenuBar::Draw(Engine* engine,AssetImporterRegistry& assetImporter)
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
                    engine->GetSceneManager().LoadScene(*engine->GetScene(), file);
                }
                
            }
            if (ImGui::MenuItem("Save scene"))
            {
                const char* filterPatterns[] = { "*.rscene" };
                const char* filePath = tinyfd_saveFileDialog("Save Scene As", "scene.rscene", 1, filterPatterns, NULL);

                std::string file = filePath ? std::string(filePath) : "";
                if (!file.empty())
                {
                    std::ofstream ofs(file);
                    if (ofs.is_open())
                    {
                        ofs.close();
                        engine->GetSceneManager().SaveScene(*engine->GetScene(), file);
                    }
                }
            }
            if (ImGui::MenuItem("Import assets"))
            {
                const char* filterPatterns[] = { "*.png", "*.jpeg", "*.jpg", "*.tga" };
                char const* lTheOpenFileName = tinyfd_openFileDialog(
                    "Select assets to import",
                    "",
                    4,
                    filterPatterns,
                    NULL,
                    1); //multi-select

                if (lTheOpenFileName)
                {
                    // tinyfd returns paths separated by '|'
                    std::string pathsStr(lTheOpenFileName);
                    size_t start = 0;
                    size_t end = pathsStr.find('|');
                    while (end != std::string::npos)
                    {
                        std::string file = pathsStr.substr(start, end - start);
                        assetImporter.Import(file);
                        start = end + 1;
                        end = pathsStr.find('|', start);
                    }
                    // Last file (or only file if no '|')
                    std::string lastFile = pathsStr.substr(start);
                    if (!lastFile.empty())
                        assetImporter.Import(lastFile);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit")) 
            {
                glfwSetWindowShouldClose(engine->GetWindow().GetGLFWWindow(), true);
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