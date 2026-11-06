#include "MenuBar.h"
#include "Core/PakWriter.h"
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
#include <thread>
#include <atomic>
#include <mutex>

namespace {

struct StandaloneExportState
{
    bool isExporting = false;
    bool shouldOpenPopup = false;
    std::atomic<float> progress{0.0f};
    std::mutex mutex;
    std::string currentFile;
    bool finished = false;
    bool success = false;
    std::string errorMessage;
    std::thread worker;

    ~StandaloneExportState()
    {
        if (worker.joinable())
            worker.join();
    }
};

static StandaloneExportState s_ExportState;

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma pack(push, 2)
struct IcoHeader {
    uint16_t reserved;
    uint16_t type;
    uint16_t count;
};
struct IcoEntry {
    uint8_t  width;
    uint8_t  height;
    uint8_t  colorCount;
    uint8_t  reserved;
    uint16_t planes;
    uint8_t  bitCountLo;
    uint8_t  bitCountHi;
    uint32_t bytesInRes;
    uint32_t imageOffset;
};
struct GrpIconEntry {
    uint8_t  width;
    uint8_t  height;
    uint8_t  colorCount;
    uint8_t  reserved;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t bytesInRes;
    uint16_t id;
};
struct GrpIconHeader {
    uint16_t reserved;
    uint16_t type;
    uint16_t count;
};
#pragma pack(pop)

static bool UpdateExecutableIcon(const std::filesystem::path& exePath, const std::filesystem::path& iconPath)
{
    if (!std::filesystem::exists(exePath) || !std::filesystem::exists(iconPath))
        return false;

    std::ifstream file(iconPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    if (fileSize <= 6) return false;

    std::vector<uint8_t> data(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(data.data()), fileSize))
        return false;

    HANDLE hUpdate = BeginUpdateResourceW(exePath.wstring().c_str(), FALSE);
    if (!hUpdate) return false;

    bool isIco = (data.size() > 4 && data[0] == 0 && data[1] == 0 && data[2] == 1 && data[3] == 0);
    if (isIco)
    {
        const IcoHeader* hdr = reinterpret_cast<const IcoHeader*>(data.data());
        uint16_t count = hdr->count;
        if (data.size() < sizeof(IcoHeader) + count * sizeof(IcoEntry))
        {
            EndUpdateResourceW(hUpdate, TRUE);
            return false;
        }

        const IcoEntry* entries = reinterpret_cast<const IcoEntry*>(data.data() + sizeof(IcoHeader));
        std::vector<uint8_t> grpData(sizeof(GrpIconHeader) + count * sizeof(GrpIconEntry));
        GrpIconHeader* grpHdr = reinterpret_cast<GrpIconHeader*>(grpData.data());
        grpHdr->reserved = 0;
        grpHdr->type = 1;
        grpHdr->count = count;

        GrpIconEntry* grpEntries = reinterpret_cast<GrpIconEntry*>(grpData.data() + sizeof(GrpIconHeader));

        for (uint16_t i = 0; i < count; ++i)
        {
            if (entries[i].imageOffset + entries[i].bytesInRes > data.size())
                continue;

            UpdateResourceW(hUpdate, RT_ICON, MAKEINTRESOURCEW(i + 1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                data.data() + entries[i].imageOffset, entries[i].bytesInRes);

            grpEntries[i].width = entries[i].width;
            grpEntries[i].height = entries[i].height;
            grpEntries[i].colorCount = entries[i].colorCount;
            grpEntries[i].reserved = entries[i].reserved;
            grpEntries[i].planes = entries[i].planes;
            grpEntries[i].bitCount = static_cast<uint16_t>(entries[i].bitCountLo | (entries[i].bitCountHi << 8));
            grpEntries[i].bytesInRes = entries[i].bytesInRes;
            grpEntries[i].id = i + 1;
        }

        UpdateResourceW(hUpdate, RT_GROUP_ICON, MAKEINTRESOURCEW(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            grpData.data(), static_cast<DWORD>(grpData.size()));
        EndUpdateResourceW(hUpdate, FALSE);
        return true;
    }
    else
    {
        // Treat as PNG icon entry
        std::vector<uint8_t> grpData(sizeof(GrpIconHeader) + sizeof(GrpIconEntry));
        GrpIconHeader* grpHdr = reinterpret_cast<GrpIconHeader*>(grpData.data());
        grpHdr->reserved = 0;
        grpHdr->type = 1;
        grpHdr->count = 1;

        GrpIconEntry* grpEntries = reinterpret_cast<GrpIconEntry*>(grpData.data() + sizeof(GrpIconHeader));
        grpEntries[0].width = 0;
        grpEntries[0].height = 0;
        grpEntries[0].colorCount = 0;
        grpEntries[0].reserved = 0;
        grpEntries[0].planes = 1;
        grpEntries[0].bitCount = 32;
        grpEntries[0].bytesInRes = static_cast<uint32_t>(data.size());
        grpEntries[0].id = 1;

        UpdateResourceW(hUpdate, RT_ICON, MAKEINTRESOURCEW(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            data.data(), static_cast<DWORD>(data.size()));
        UpdateResourceW(hUpdate, RT_GROUP_ICON, MAKEINTRESOURCEW(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            grpData.data(), static_cast<DWORD>(grpData.size()));
        EndUpdateResourceW(hUpdate, FALSE);
        return true;
    }
}
#endif

static std::filesystem::path FindRuntimeExecutable()
{
    return rv::EditorUtils::FindRuntimeExecutable();
}

}

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

            if (ImGui::MenuItem("Switch Project"))
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

            if (ImGui::MenuItem("Run in Runtime", "Ctrl+F5", false, ProjectManager::IsProjectLoaded()))
            {
                std::string err;
                if (!EditorUtils::LaunchActiveProject(*engine, &err))
                {
                    LOG_ERROR("Failed to launch runtime: {}", err);
                }
            }

            if (ImGui::MenuItem("Export project", nullptr, false, ProjectManager::IsProjectLoaded() && !s_ExportState.isExporting))
            {
                auto activeProj = ProjectManager::GetActiveProject();
                if (activeProj)
                {
                    std::string chosen = Dialogs::SelectFolderDialog("Select Export Directory");
                    if (!chosen.empty())
                    {
                        if (s_ExportState.worker.joinable())
                            s_ExportState.worker.join();

                        s_ExportState.isExporting = true;
                        s_ExportState.shouldOpenPopup = true;
                        s_ExportState.progress.store(0.0f);
                        s_ExportState.finished = false;
                        s_ExportState.success = false;
                        s_ExportState.errorMessage.clear();
                        {
                            std::lock_guard<std::mutex> lock(s_ExportState.mutex);
                            s_ExportState.currentFile = "Preparing export...";
                        }

                        std::filesystem::path outFolder = chosen;
                        std::filesystem::path projPath = activeProj->projectFolderPath;
                        std::string projName = activeProj->name;
                        std::string projIcon = activeProj->GetSettings().iconPath;

                        s_ExportState.worker = std::thread([outFolder, projPath, projName, projIcon]() {
                            std::filesystem::path pakDest = outFolder / (projName + ".pak");

                            PakBuildOptions options;
                            options.projectPath = projPath;
                            options.outputPakPath = pakDest;
                            options.includeEngineResources = true;
                            options.progressCallback = [](const std::string& currentFile, float progress) {
                                s_ExportState.progress.store(progress);
                                std::lock_guard<std::mutex> lock(s_ExportState.mutex);
                                s_ExportState.currentFile = currentFile;
                            };

                            auto u8 = pakDest.u8string();
                            std::string pakDestStr(reinterpret_cast<const char*>(u8.data()), u8.size());
                            PakBuildResult pakResult = PakWriter::BuildPak(options);

                            if (pakResult.success)
                            {
                                {
                                    std::lock_guard<std::mutex> lock(s_ExportState.mutex);
                                    s_ExportState.currentFile = "Copying runtime executable...";
                                }
                                std::filesystem::path runtimeSrc = FindRuntimeExecutable();
                                std::filesystem::path runtimeDst = outFolder / (projName + ".exe");

                                if (runtimeSrc.empty())
                                {
                                    s_ExportState.success = false;
                                    s_ExportState.errorMessage = "RvelaRuntime.exe not found! Please build the RvelaRuntime project before exporting.";
                                }
                                else
                                {
                                    std::error_code copyEc;
                                    std::filesystem::copy_file(runtimeSrc, runtimeDst, std::filesystem::copy_options::overwrite_existing, copyEc);
                                    if (copyEc)
                                    {
                                        s_ExportState.success = false;
                                        s_ExportState.errorMessage = "Failed to copy runtime executable: " + copyEc.message();
                                    }
                                    else
                                    {
#if defined(_WIN32)
                                        // Update exported executable icon
                                        std::filesystem::path iconToApply;
                                        if (!projIcon.empty())
                                        {
                                            if (std::filesystem::exists(projPath / projIcon))
                                                iconToApply = projPath / projIcon;
                                            else if (std::filesystem::exists(projIcon))
                                                iconToApply = projIcon;
                                        }
                                        if (iconToApply.empty())
                                        {
                                            const std::vector<std::string> candidates = { "icon.ico", "icon.png", "Assets/icon.ico", "Assets/icon.png" };
                                            for (const auto& c : candidates)
                                            {
                                                if (std::filesystem::exists(projPath / c))
                                                {
                                                    iconToApply = projPath / c;
                                                    break;
                                                }
                                            }
                                        }
                                        if (!iconToApply.empty() && std::filesystem::exists(runtimeDst))
                                        {
                                            UpdateExecutableIcon(runtimeDst, iconToApply);
                                        }
#endif
                                        s_ExportState.progress.store(1.0f);
                                        s_ExportState.success = true;
                                    }
                                }
                            }
                            else
                            {
                                s_ExportState.success = false;
                                s_ExportState.errorMessage = pakResult.errorMessage;
                            }

                            s_ExportState.finished = true;
                        });
                    }
                }
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

    if (s_ExportState.isExporting && s_ExportState.shouldOpenPopup)
    {
        ImGui::OpenPopup("Exporting Standalone Game##Modal");
        s_ExportState.shouldOpenPopup = false;
    }

    if (ImGui::BeginPopupModal("Exporting Standalone Game##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!s_ExportState.finished)
        {
            ImGui::Text("Exporting game package, please wait...");
            ImGui::Spacing();

            float prog = s_ExportState.progress.load();
            ImGui::ProgressBar(prog, ImVec2(400.0f, 24.0f));

            std::string curFile;
            {
                std::lock_guard<std::mutex> lock(s_ExportState.mutex);
                curFile = s_ExportState.currentFile;
            }
            ImGui::TextDisabled("%s", curFile.c_str());
        }
        else
        {
            if (s_ExportState.worker.joinable())
                s_ExportState.worker.join();

            if (s_ExportState.success)
            {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Game exported successfully!");
                ImGui::Spacing();
                if (ImGui::Button("OK", ImVec2(120.0f, 28.0f)))
                {
                    s_ExportState.isExporting = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Export failed: %s", s_ExportState.errorMessage.c_str());
                ImGui::Spacing();
                if (ImGui::Button("Close", ImVec2(120.0f, 28.0f)))
                {
                    s_ExportState.isExporting = false;
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        ImGui::EndPopup();
    }
}
