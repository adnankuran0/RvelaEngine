#include "MenuBar.h"
#include "Core/Engine.h"
#include "Asset/AssetImportPipeline.h"
#include "Asset/AssetRegistry.h"
#include "AssetImporters/MaterialSerializer.h"
#include <imgui.h>
#include "Dialogs.h"
#include "EditorFileDialog.h"
#include "EditorUtils.h"
#include "tinyfiledialogs.h"
#include "Asset/AssetManager.h"
#include "Utils/ProjectManager.h"
#include "Panels/ProjectSettingsPanel.h"
#include "Panels/ProjectSelectorPanel.h"
#include "EditorSettings.h"
#include "Commands/CommandHistory.h"
#include "Renderer/RenderLayer.h"
#include "Panels/EditorPanel.h"
#include "Panels/ImportSettingsPanel.h"
#include "Panels/MaterialEditorPanel.h"
#include "Viewport.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/EnvironmentPanel.h"
#include "Panels/AssetBrowserPanel.h"
#include "Panels/MixerPanel.h"
#include "Panels/AnimatorPanel.h"
#include "Panels/ConsolePanel.h"
#include "PrefabEditManager.h"
#include "Style.h"

using namespace rv;

void MenuBar::Draw(Engine* engine, AssetImportPipeline& assetImporter, const EditorPanels& panels)
{
    auto& registry = AssetManager::Get().GetRegistry();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (PrefabEditManager::Get().IsInPrefabMode())
            {
                if (ImGui::MenuItem("Save Prefab", "Ctrl+S"))
                    PrefabEditManager::Get().SaveCurrentPrefab(*engine);

                if (ImGui::MenuItem("Return to Scene", "Esc"))
                    PrefabEditManager::Get().ExitPrefabMode(*engine, true);

                ImGui::Separator();
            }
            else
            {
                if (ImGui::MenuItem("New scene", "Ctrl+N"))
                    EditorUtils::CreateScene(*engine);

                if (ImGui::MenuItem("Open scene", "Ctrl+O"))
                    EditorUtils::OpenScene(*engine);

                if (ImGui::MenuItem("Save scene", "Ctrl+S"))
                    EditorUtils::SaveScene(*engine);

                if (ImGui::MenuItem("Save scene as", "Ctrl+Shift+S"))
                    EditorUtils::SaveSceneAs(*engine);

                ImGui::Separator();
            }

            if (ImGui::MenuItem("Switch Project..."))
            {
                if (panels.projectSelector)
                    panels.projectSelector->Open();
            }

            if (ImGui::MenuItem("Open VS Code Workspace", nullptr, false, ProjectManager::IsProjectLoaded()))
                EditorUtils::OpenVSCodeWorkspace();

            if (ImGui::MenuItem("Save Project"))
            {
                ProjectManager::SaveActiveProject();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Import assets"))
            {
                const char* filterPatterns[] = {
                    "*.png", "*.jpeg", "*.jpg", "*.tga",
                    "*.fbx", "*.gltf", "*.obj", "*.glb"
                };
                std::string defaultPath = ProjectManager::IsProjectLoaded()
                    ? ProjectManager::GetProjectPath().string()
                    : "";
                const char* selected = tinyfd_openFileDialog(
                    "Select assets to import", defaultPath.c_str(), 8, filterPatterns, NULL, 1);

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

            if (ImGui::MenuItem("Reimport all assets"))
            {
                assetImporter.ReimportAll(AssetManager::Get().GetRegistry());
            }

            if (ImGui::MenuItem("Create Material"))
            {
                std::filesystem::path initialDir = ProjectManager::GetAssetDirectory();
                if (std::filesystem::exists(initialDir / "Materials"))
                    initialDir /= "Materials";

                EditorFileDialog::Open(
                    "Create Material",
                    FileDialogMode::SaveFile,
                    { ".rmat" },
                    initialDir,
                    [&registry](const std::filesystem::path& chosen) {
                        MaterialSerializer::CreateNew(chosen, registry);
                        registry.Scan(registry.GetAssetDir());
                    },
                    "NewMaterial.rmat"
                );
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Quit"))
                glfwSetWindowShouldClose(engine->GetWindow().GetGLFWWindow(), true);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            const bool isEditMode = (engine && engine->GetActiveScene().GetState() == SceneState::EDIT);

            std::string undoLabel = "Undo";
            if (CommandHistory::Get().CanUndo())
                undoLabel += " " + CommandHistory::Get().GetUndoName();
            if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, isEditMode && CommandHistory::Get().CanUndo()))
            {
                if (isEditMode)
                    CommandHistory::Get().Undo();
            }

            std::string redoLabel = "Redo";
            if (CommandHistory::Get().CanRedo())
                redoLabel += " " + CommandHistory::Get().GetRedoName();
            if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, isEditMode && CommandHistory::Get().CanRedo()))
            {
                if (isEditMode)
                    CommandHistory::Get().Redo();
            }

            ImGui::Separator();


            if (ImGui::MenuItem("Project Settings..."))
            {
                if (panels.projectSettings)
                    panels.projectSettings->Open();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            const bool hasTransformSelection = EditorUtils::HasTransformSelection(*engine);
            if (ImGui::MenuItem("Align Transform with View", "Ctrl+Alt+M", false, hasTransformSelection))
            {
                EditorUtils::AlignSelectedWithView(*engine);
            }
            if (ImGui::MenuItem("Snap to Floor", "PageDown", false, hasTransformSelection))
            {
                EditorUtils::SnapSelectedToFloor(*engine, false);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Reset Layout"))
            {
                if (panels.onResetLayout)
                    panels.onResetLayout();
            }

            ImGui::Separator();

            for (EditorPanel* panel : panels.viewPanels)
            {
                if (!panel) continue;
                bool isOpen = panel->IsOpen();
                if (ImGui::MenuItem(panel->GetName(), nullptr, &isOpen))
                {
                    panel->SetOpen(isOpen);
                    EditorSettings::Get().Panels[panel->GetName()] = isOpen;
                    EditorSettings::Get().Save();
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Theme"))
        {
            EditorTheme currentTheme = GetCurrentTheme();
            auto ThemeItem = [&](const char* name, EditorTheme theme) {
                bool selected = (currentTheme == theme);
                if (ImGui::MenuItem(name, nullptr, selected))
                {
                    ApplyTheme(theme);
                    EditorSettings::Get().Theme = static_cast<int>(theme);
                    EditorSettings::Get().Save();
                }
            };

            ThemeItem("Deep Slate (Midnight)", EditorTheme::DeepSlate);
            ThemeItem("Tokyo Night (Violet)", EditorTheme::TokyoNight);
            ThemeItem("Catppuccin (Mocha)", EditorTheme::Catppuccin);
            ThemeItem("Obsidian (Carbon)", EditorTheme::ObsidianCarbon);
            ThemeItem("Warm Charcoal (Amber)", EditorTheme::WarmCharcoal);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
