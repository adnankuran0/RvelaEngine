#include "ProjectSettingsPanel.h"
#include "Core/Engine.h"
#include "Utils/ProjectManager.h"
#include "GUI/Dialogs.h"
#include "GUI/EditorFileDialog.h"
#include "GUI/Style.h"
#include "Core/Time.h"
#include <imgui.h>
#include <filesystem>

using namespace rv;

void ProjectSettingsPanel::Draw(Engine* engine)
{
    if (!m_IsOpen)
    {
        m_Initialized = false;
        return;
    }

    auto activeProj = ProjectManager::GetActiveProject();
    if (!activeProj)
    {
        m_IsOpen = false;
        m_Initialized = false;
        return;
    }

    auto& settings = activeProj->GetSettings();

    if (!m_Initialized)
    {
        strncpy_s(m_NameBuf, settings.name.c_str(), sizeof(m_NameBuf) - 1);
        strncpy_s(m_StartSceneBuf, settings.startScene.c_str(), sizeof(m_StartSceneBuf) - 1);
        for (int i = 0; i < 32; i++)
        {
            strncpy_s(m_PhysicsLayerBuf[i], settings.physicsLayerNames[i].c_str(), sizeof(m_PhysicsLayerBuf[i]) - 1);
        }
        m_Initialized = true;
    }

    ImGui::SetNextWindowSize(ImVec2(640, 520), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Project Settings", &m_IsOpen))
    {
        if (ImGui::BeginTabBar("ProjectSettingsTabs"))
        {
            if (ImGui::BeginTabItem("General"))
            {
                ImGui::Spacing();
                ImGui::TextColored(GetAccentColor(), "General Settings");
                ImGui::Separator();
                ImGui::Spacing();

        if (ImGui::InputText("Project Name", m_NameBuf, sizeof(m_NameBuf)))
        {
            settings.name = m_NameBuf;
            activeProj->name = m_NameBuf;
        }

        ImGui::Spacing();
        ImGui::Text("Start Scene:");
        if (ImGui::InputText("##StartScene", m_StartSceneBuf, sizeof(m_StartSceneBuf)))
        {
            settings.startScene = m_StartSceneBuf;
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                std::filesystem::path droppedPath = (const char*)payload->Data;
                if (droppedPath.extension() == ".rscene")
                {
                    std::error_code ec;
                    std::filesystem::path rel = std::filesystem::relative(droppedPath, ProjectManager::GetProjectPath(), ec);
                    std::string relStr = (!ec && !rel.empty()) ? rel.generic_string() : droppedPath.generic_string();
                    settings.startScene = relStr;
                    strncpy_s(m_StartSceneBuf, relStr.c_str(), sizeof(m_StartSceneBuf) - 1);
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine();
        if (ImGui::Button("Browse##Scene"))
        {
            std::filesystem::path startDir = ProjectManager::GetAssetDirectory();
            if (std::filesystem::exists(startDir / "Scenes"))
                startDir /= "Scenes";

            EditorFileDialog::Open(
                "Select Start Scene",
                FileDialogMode::OpenFile,
                { ".rscene" },
                startDir,
                [this, activeProj](const std::filesystem::path& chosen) {
                    std::error_code ec;
                    std::filesystem::path rel = std::filesystem::relative(chosen, ProjectManager::GetProjectPath(), ec);
                    std::string relStr = (!ec && !rel.empty()) ? rel.generic_string() : chosen.generic_string();
                    activeProj->GetSettings().startScene = relStr;
                    strncpy_s(m_StartSceneBuf, relStr.c_str(), sizeof(m_StartSceneBuf) - 1);
                }
            );
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(GetAccentColor(), "Viewport");
        ImGui::Spacing();

        int viewRes[2] = { static_cast<int>(settings.viewportWidth), static_cast<int>(settings.viewportHeight) };
        if (ImGui::DragInt2("Viewport Resolution", viewRes, 1.0f, 160, 7680))
        {
            settings.viewportWidth = static_cast<uint32_t>(std::max(1, viewRes[0]));
            settings.viewportHeight = static_cast<uint32_t>(std::max(1, viewRes[1]));
        }

        const char* stretchModeOptions[] = {
            "Disabled",
            "CanvasItems",
            "Viewport"
        };
        int currentStretch = static_cast<int>(settings.stretchMode);
        if (ImGui::Combo("Stretch Mode", &currentStretch, stretchModeOptions, IM_ARRAYSIZE(stretchModeOptions)))
        {
            settings.stretchMode = static_cast<StretchMode>(currentStretch);
        }

        const char* stretchAspectOptions[] = {
            "Ignore (Stretch to fit)",
            "Keep (Letterbox)",
            "Keep Width",
            "Keep Height",
            "Expand"
        };
        int currentAspect = static_cast<int>(settings.stretchAspect);
        if (ImGui::Combo("Stretch Aspect", &currentAspect, stretchAspectOptions, IM_ARRAYSIZE(stretchAspectOptions)))
        {
            settings.stretchAspect = static_cast<StretchAspect>(currentAspect);
        }

        const char* scalingFilterOptions[] = {
            "Linear",
            "Nearest"
        };
        int currentFilter = static_cast<int>(settings.scalingFilter);
        if (ImGui::Combo("Scaling Filter", &currentFilter, scalingFilterOptions, IM_ARRAYSIZE(scalingFilterOptions)))
        {
            settings.scalingFilter = static_cast<ScalingFilter>(currentFilter);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(GetAccentColor(), "Window");
        ImGui::Spacing();

        int winSize[2] = { static_cast<int>(settings.windowWidth), static_cast<int>(settings.windowHeight) };
        if (ImGui::DragInt2("Window Size", winSize, 1.0f, 320, 7680))
        {
            settings.windowWidth = static_cast<uint32_t>(std::max(1, winSize[0]));
            settings.windowHeight = static_cast<uint32_t>(std::max(1, winSize[1]));
        }

        const char* windowModeOptions[] = { "Windowed", "Minimized", "Maximized", "Fullscreen" };
        int currentWinMode = static_cast<int>(settings.windowMode);
        if (ImGui::Combo("Window Mode", &currentWinMode, windowModeOptions, IM_ARRAYSIZE(windowModeOptions)))
        {
            settings.windowMode = static_cast<WindowMode>(currentWinMode);
        }

        ImGui::Checkbox("Resizable", &settings.resizable);
        ImGui::SameLine(180.0f);
        ImGui::Checkbox("Borderless", &settings.borderless);

        if (ImGui::Checkbox("VSync", &settings.vsync))
        {
            if (engine)
            {
                engine->GetWindow().SetVSync(settings.vsync);
            }
        }

        ImGui::Spacing();
        if (ImGui::SliderInt("Max FPS", &settings.maxFPS, 1,999))
        {
            settings.maxFPS = std::max(0, settings.maxFPS);
            Time::SetMaxFPS(settings.maxFPS);
        }
        ImGui::SameLine();
        if (ImGui::Button("Uncapped")) { settings.maxFPS = 0; Time::SetMaxFPS(0); }
       

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "Window Size Override:");
        ImGui::Checkbox("Override Window Size", &settings.overrideWindowSize);

        if (settings.overrideWindowSize)
        {
            int overWin[2] = { static_cast<int>(settings.windowOverrideWidth), static_cast<int>(settings.windowOverrideHeight) };
            if (ImGui::DragInt2("Override Window Size", overWin, 1.0f, 160, 7680))
            {
                settings.windowOverrideWidth = static_cast<uint32_t>(std::max(1, overWin[0]));
                settings.windowOverrideHeight = static_cast<uint32_t>(std::max(1, overWin[1]));
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(GetAccentColor(), "Rendering & Quality");
        ImGui::Spacing();

        const char* msaaOptions[] = { "Off (1x)", "2x MSAA", "4x MSAA", "8x MSAA" };
        int currentMsaa = 0;
        if (settings.msaaSamples == 2) currentMsaa = 1;
        else if (settings.msaaSamples == 4) currentMsaa = 2;
        else if (settings.msaaSamples == 8) currentMsaa = 3;

        if (ImGui::Combo("MSAA", &currentMsaa, msaaOptions, IM_ARRAYSIZE(msaaOptions)))
        {
            if (currentMsaa == 0) settings.msaaSamples = 1;
            else if (currentMsaa == 1) settings.msaaSamples = 2;
            else if (currentMsaa == 2) settings.msaaSamples = 4;
            else if (currentMsaa == 3) settings.msaaSamples = 8;
        }

        ImGui::Checkbox("FXAA", &settings.fxaaEnabled);

        const char* shadowResOptions[] = { "Low (1024x1024)", "Medium (2048x2048)", "High (4096x4096)" };
        int currentShadow = 1;
        if (settings.shadowResolution == 1024) currentShadow = 0;
        else if (settings.shadowResolution == 4096) currentShadow = 2;

        if (ImGui::Combo("Shadow Resolution", &currentShadow, shadowResOptions, IM_ARRAYSIZE(shadowResOptions)))
        {
            if (currentShadow == 0) settings.shadowResolution = 1024;
            else if (currentShadow == 1) settings.shadowResolution = 2048;
            else if (currentShadow == 2) settings.shadowResolution = 4096;
            ProjectManager::SaveActiveProject();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(GetAccentColor(), "Textures & Sampling");
        ImGui::Spacing();

        const char* defaultFilters[] = {
            "Nearest",
            "Linear",
            "Nearest Mipmap",
            "Linear Mipmap"
        };
        int currentDefFilter = static_cast<int>(settings.defaultTextureFilter) - 1;
        if (currentDefFilter < 0 || currentDefFilter >= 4) currentDefFilter = 3;
        if (ImGui::Combo("Default Texture Filter", &currentDefFilter, defaultFilters, IM_ARRAYSIZE(defaultFilters)))
        {
            settings.defaultTextureFilter = static_cast<TextureFilter>(currentDefFilter + 1);
            ProjectManager::SaveActiveProject();
        }

        const char* defaultWraps[] = {
            "Repeat",
            "Mirror Repeat",
            "Clamp to Edge",
            "Clamp to Border",
            "Mirror Clamp to Edge"
        };
        int currentDefWrap = static_cast<int>(settings.defaultTextureWrap) - 1;
        if (currentDefWrap < 0 || currentDefWrap >= 5) currentDefWrap = 0;
        if (ImGui::Combo("Default Texture Repeat", &currentDefWrap, defaultWraps, IM_ARRAYSIZE(defaultWraps)))
        {
            settings.defaultTextureWrap = static_cast<TextureWrap>(currentDefWrap + 1);
            ProjectManager::SaveActiveProject();
        }

        const char* defaultAnisoLevels[] = {
            "Disabled",
            "2x",
            "4x",
            "8x",
            "16x"
        };
        int currentDefAniso = static_cast<int>(settings.defaultAnisotropy) - 1;
        if (currentDefAniso < 0 || currentDefAniso >= 5) currentDefAniso = 3;
        if (ImGui::Combo("Default Anisotropy", &currentDefAniso, defaultAnisoLevels, IM_ARRAYSIZE(defaultAnisoLevels)))
        {
            settings.defaultAnisotropy = static_cast<TextureAnisotropy>(currentDefAniso + 1);
            ProjectManager::SaveActiveProject();
        }

        ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Globals"))
        {
            ImGui::Spacing();
            ImGui::TextColored(GetAccentColor(), "Autoload");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Path (.lua or .rprefab):");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);
            ImGui::InputText("##AutoloadPath", m_AutoloadPathBuf, sizeof(m_AutoloadPathBuf));

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    std::filesystem::path droppedPath = (const char*)payload->Data;
                    std::string ext = droppedPath.extension().string();
                    if (ext == ".lua" || ext == ".rprefab")
                    {
                        std::error_code ec;
                        std::filesystem::path rel = std::filesystem::relative(droppedPath, ProjectManager::GetProjectPath(), ec);
                        std::string relStr = (!ec && !rel.empty()) ? rel.generic_string() : droppedPath.generic_string();
                        strncpy_s(m_AutoloadPathBuf, relStr.c_str(), sizeof(m_AutoloadPathBuf) - 1);
                        if (m_AutoloadNameBuf[0] == '\0')
                        {
                            std::string stem = droppedPath.stem().string();
                            strncpy_s(m_AutoloadNameBuf, stem.c_str(), sizeof(m_AutoloadNameBuf) - 1);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine();
            if (ImGui::Button("Browse##Autoload"))
            {
                EditorFileDialog::Open(
                    "Select Autoload Script or Prefab",
                    FileDialogMode::OpenFile,
                    { ".lua", ".rprefab" },
                    ProjectManager::GetAssetDirectory(),
                    [this](const std::filesystem::path& chosen) {
                        std::error_code ec;
                        std::filesystem::path rel = std::filesystem::relative(chosen, ProjectManager::GetProjectPath(), ec);
                        std::string relStr = (!ec && !rel.empty()) ? rel.generic_string() : chosen.generic_string();
                        strncpy_s(m_AutoloadPathBuf, relStr.c_str(), sizeof(m_AutoloadPathBuf) - 1);
                        if (m_AutoloadNameBuf[0] == '\0')
                        {
                            std::string stem = chosen.stem().string();
                            strncpy_s(m_AutoloadNameBuf, stem.c_str(), sizeof(m_AutoloadNameBuf) - 1);
                        }
                    }
                );
            }

            ImGui::Text("Singleton Name:");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100.0f);
            ImGui::InputText("##AutoloadName", m_AutoloadNameBuf, sizeof(m_AutoloadNameBuf));
            ImGui::SameLine();
            if (ImGui::Button("Add##AutoloadBtn", ImVec2(90.0f, 0.0f)))
            {
                std::string name = m_AutoloadNameBuf;
                std::string path = m_AutoloadPathBuf;
                if (!name.empty() && !path.empty())
                {
                    bool exists = false;
                    for (const auto& a : settings.autoloads)
                    {
                        if (a.name == name) { exists = true; break; }
                    }
                    if (!exists)
                    {
                        settings.autoloads.push_back(AutoloadEntry{ name, path, true });
                        m_AutoloadNameBuf[0] = '\0';
                        m_AutoloadPathBuf[0] = '\0';
                        m_AutoloadError.clear();
                    }
                    else
                    {
                        m_AutoloadError = "An autoload with this name already exists!";
                    }
                }
                else
                {
                    m_AutoloadError = "Name and Path cannot be empty!";
                }
            }

            if (!m_AutoloadError.empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", m_AutoloadError.c_str());
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Registered Autoloads (%zu):", settings.autoloads.size());

            if (settings.autoloads.empty())
            {
                ImGui::TextDisabled("No autoload singletons defined yet.");
            }
            else
            {
                if (ImGui::BeginTable("AutoloadTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                    ImGui::TableHeadersRow();

                    int toDelete = -1;
                    for (int i = 0; i < static_cast<int>(settings.autoloads.size()); ++i)
                    {
                        auto& entry = settings.autoloads[i];
                        ImGui::TableNextRow();

                        // Name
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(entry.name.c_str());

                        // Type
                        ImGui::TableSetColumnIndex(1);
                        std::string ext = std::filesystem::path(entry.path).extension().string();
                        if (ext == ".rprefab" || ext == ".prefab")
                            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Prefab");
                        else if (ext == ".lua")
                            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.6f, 1.0f), "Script");
                        else
                            ImGui::TextDisabled("Unknown");

                        // Path
                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(entry.path.c_str());

                        // Enabled
                        ImGui::TableSetColumnIndex(3);
                        std::string enableId = "##en_" + std::to_string(i);
                        ImGui::Checkbox(enableId.c_str(), &entry.enabled);

                        // Delete button
                        ImGui::TableSetColumnIndex(4);
                        std::string delId = "Delete##del_" + std::to_string(i);
                        if (ImGui::Button(delId.c_str()))
                        {
                            toDelete = i;
                        }
                    }

                    if (toDelete >= 0 && toDelete < static_cast<int>(settings.autoloads.size()))
                    {
                        settings.autoloads.erase(settings.autoloads.begin() + toDelete);
                    }

                    ImGui::EndTable();
                }
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Physics Layers"))
        {
            ImGui::Spacing();
            ImGui::TextColored(GetAccentColor(), "Physics 3D Layer Names");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Columns(2, "LayerColumns", false);
            for (int col = 0; col < 2; col++)
            {
                for (int row = 0; row < 16; row++)
                {
                    int i = col * 16 + row;
                    std::string label = "Layer " + std::to_string(i + 1);
                    ImGui::Text("%s:", label.c_str());
                    ImGui::SameLine(70.0f);
                    std::string inputId = "##PhysicsLayer" + std::to_string(i + 1);
                    ImGui::SetNextItemWidth(-10.0f);
                    if (ImGui::InputText(inputId.c_str(), m_PhysicsLayerBuf[i], sizeof(m_PhysicsLayerBuf[i])))
                    {
                        settings.physicsLayerNames[i] = m_PhysicsLayerBuf[i];
                    }
                }
                ImGui::NextColumn();
            }
            ImGui::Columns(1);

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save Settings", ImVec2(130.0f, 28.0f)))
        {
            ProjectManager::SaveActiveProject();
        }

        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(90.0f, 28.0f)))
        {
            m_IsOpen = false;
        }
    }
    ImGui::End();
}
