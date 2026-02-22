#include "MenuBar.h"
#include <ImGui/tinyfiledialogs.h>
#include <fstream>
#include <AssetImporter/MaterialImporter.h>
#include "Core/Engine.h"
#include "AssetImporter/AssetImporterRegistry.h"
#include <imgui.h>

using namespace rv;

void MenuBar::Draw(Engine* engine, AssetImporterRegistry& assetImporter)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New scene"))
            {
                auto& sm = engine->GetSceneManager();
                sm.SetActiveScene(sm.CreateScene("NewScene"));
            }
            if (ImGui::MenuItem("Open scene"))
            {
                const char* filterPatterns[] = { "*.rscene" };
                const char* filePath = tinyfd_openFileDialog("Select a scene", "", 1, filterPatterns, NULL, 0);

                std::string file = filePath ? std::string(filePath) : "";
                if (!file.empty())
                {
                    engine->GetSceneManager().LoadScene(file);
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
                        engine->GetSceneManager().SaveScene(file);
                    }
                }
            }
            if (ImGui::MenuItem("Import assets"))
            {
                const char* filterPatterns[] = { "*.png", "*.jpeg", "*.jpg", "*.tga", "*.fbx", "*.gltf", "*.obj", "*.glb"};
                char const* lTheOpenFileName = tinyfd_openFileDialog(
                    "Select assets to import",
                    "",
                    8,
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
                    {
                        assetImporter.Import(lastFile);
                    }

                }
            }
            if (ImGui::MenuItem("Create Material"))
            {
                const char* filterPatterns[] = { "*.rmat" };
                const char* filePath = tinyfd_saveFileDialog("Create material as", "material.rmat", 1, filterPatterns, NULL);

                std::string file = filePath ? std::string(filePath) : "";
                if (!file.empty())
                {
                    std::ofstream ofs(file);
                    if (ofs.is_open())
                    {
                        ofs.close();
                        MaterialImporter::Get().CreateMaterialAsset(file);
                        AssetRegistry::ScanAssets();
                    }
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

