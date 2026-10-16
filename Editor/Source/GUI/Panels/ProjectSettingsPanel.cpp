#include "ProjectSettingsPanel.h"
#include "Core/Engine.h"
#include "Utils/ProjectManager.h"
#include "GUI/Dialogs.h"
#include "GUI/EditorFileDialog.h"
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
        m_Initialized = true;
    }

    ImGui::SetNextWindowSize(ImVec2(520, 360), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Project Settings", &m_IsOpen))
    {
        ImGui::TextColored(ImVec4(0.62f, 0.56f, 0.80f, 1.0f), "General Settings");
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
        ImGui::TextColored(ImVec4(0.62f, 0.56f, 0.80f, 1.0f), "Viewport");
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
        ImGui::TextColored(ImVec4(0.62f, 0.56f, 0.80f, 1.0f), "Window");
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
        ImGui::TextColored(ImVec4(0.62f, 0.56f, 0.80f, 1.0f), "Rendering & Quality");
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
