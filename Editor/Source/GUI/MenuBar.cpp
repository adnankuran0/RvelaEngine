#include "MenuBar.h"
#include "Core/Engine.h"
#include "Asset/AssetImportPipeline.h"
#include "Asset/AssetRegistry.h"
#include "AssetImporters/MaterialSerializer.h"
#include <imgui.h>
#include "Dialogs.h"
#include "EditorUtils.h"
#include "tinyfiledialogs.h"
#include "Asset/AssetManager.h"

using namespace rv;

void MenuBar::Draw(Engine* engine, AssetImportPipeline& assetImporter)
{
    auto& registry = AssetManager::Get().GetRegistry();


    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New scene (Ctrl + N)"))
                EditorUtils::CreateScene(*engine);

            if (ImGui::MenuItem("Open scene (Ctrl + O)"))
                EditorUtils::OpenScene(*engine);

            if (ImGui::MenuItem("Save scene (Ctrl + S)"))
                EditorUtils::SaveScene(*engine);

            if (ImGui::MenuItem("Save scene as (Ctrl + Shift + S)"))
                EditorUtils::SaveSceneAs(*engine);

            if (ImGui::MenuItem("Import assets"))
            {
                const char* filterPatterns[] = {
                    "*.png", "*.jpeg", "*.jpg", "*.tga",
                    "*.fbx", "*.gltf", "*.obj", "*.glb"
                };
                const char* selected = tinyfd_openFileDialog(
                    "Select assets to import", "", 8, filterPatterns, NULL, 1);


                if (selected)
                {
                    std::string pathsStr(selected);
                    size_t start = 0;
                    size_t end = pathsStr.find('|');

                    while (end != std::string::npos)
                    {
                        assetImporter.ImportAsset(pathsStr.substr(start, end - start), registry);
                        start = end + 1;
                        end = pathsStr.find('|', start);
                    }

                    std::string lastFile = pathsStr.substr(start);
                    if (!lastFile.empty())
                        assetImporter.ImportAsset(lastFile, registry);

                    registry.Scan(registry.GetAssetDir());
                }
            }

            if (ImGui::MenuItem("Create Material"))
            {
                const char* filterPatterns[] = { "*.rmat" };
                const char* filePath = tinyfd_saveFileDialog(
                    "Create material as", "material.rmat", 1, filterPatterns, NULL);

                if (filePath)
                {
                    std::filesystem::path matPath = filePath;
                    MaterialSerializer::CreateNew(matPath, registry);
                    registry.Scan(registry.GetAssetDir());
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Quit"))
                glfwSetWindowShouldClose(engine->GetWindow().GetGLFWWindow(), true);

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