#include "MenuBar.h"

#include <AssetImporter/MaterialImporter.h>
#include "Core/Engine.h"
#include "AssetImporter/AssetImporterRegistry.h"
#include <imgui.h>
#include "Dialogs.h"
#include "EditorUtils.h"
#include "tinyfiledialogs.h"

using namespace rv;

void MenuBar::Draw(Engine* engine, AssetImporterRegistry& assetImporter)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New scene (Ctrl + N)"))
            {
                EditorUtils::CreateScene(*engine);
            }
            if (ImGui::MenuItem("Open scene (Ctrl + O)"))
            {
                EditorUtils::OpenScene(*engine);
            }
            if (ImGui::MenuItem("Save scene (Ctrl + S)"))
            {
                EditorUtils::SaveScene(*engine);
            }
            if (ImGui::MenuItem("Save scene as (Ctrl + Shift + S)"))
            {
                EditorUtils::SaveSceneAs(*engine);
               
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
                    1); 

                if (lTheOpenFileName)
                {
                    // tinyfd returns paths separated by |
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
                    // Last file (or only file if no |)
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

