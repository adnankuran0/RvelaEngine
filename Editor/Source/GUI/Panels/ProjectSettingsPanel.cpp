#include "ProjectSettingsPanel.h"
#include "Core/Engine.h"
#include "Utils/ProjectManager.h"
#include "GUI/Dialogs.h"
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
        ImGui::Text("Start Scene (Relative to project root):");
        if (ImGui::InputText("##StartScene", m_StartSceneBuf, sizeof(m_StartSceneBuf)))
        {
            settings.startScene = m_StartSceneBuf;
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse##Scene"))
        {
            std::string scene = Dialogs::OpenSceneDialog();
            if (!scene.empty())
            {
                std::filesystem::path rel = std::filesystem::relative(scene, ProjectManager::GetProjectPath());
                settings.startScene = rel.string();
                strncpy_s(m_StartSceneBuf, settings.startScene.c_str(), sizeof(m_StartSceneBuf) - 1);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.62f, 0.56f, 0.80f, 1.0f), "Display & Window");
        ImGui::Spacing();

        int res[2] = { static_cast<int>(settings.windowWidth), static_cast<int>(settings.windowHeight) };
        if (ImGui::DragInt2("Default Resolution", res, 1.0f, 640, 7680))
        {
            settings.windowWidth = static_cast<uint32_t>(res[0]);
            settings.windowHeight = static_cast<uint32_t>(res[1]);
        }

        if (ImGui::Checkbox("VSync", &settings.vsync))
        {
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
