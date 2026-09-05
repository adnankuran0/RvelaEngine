#include "EnvironmentPanel.h"
#include "Core/Engine.h"
#include "ImGui/imgui.h"
#include "glm/glm.hpp"

using namespace rv;

void EnvironmentPanel::Draw(Engine* engine)
{
    ImGui::Begin("Environment");

    auto& env = engine->GetActiveScene().GetEnvironment();

    if (ImGui::CollapsingHeader("Lighting"))
    {
        ImGui::Button("Skybox HDR", ImVec2(200, 20));
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                const char* path = (const char*)payload->Data;
                std::string pathStr(path);
                if (pathStr.ends_with(".hdr"))
                {
                    Path absolutePath = Path::FromAbsolute(pathStr);
                    env.GetSkybox().InitHDR(absolutePath);
                }
            }
            ImGui::EndDragDropTarget();
        }

        bool* useIBL = &env.Lighting_IBL;
        ImGui::Checkbox("Use IBL", useIBL);

        if (*useIBL)
        {
            ImGui::SliderFloat("IBL Intensity", &env.Lighting_IBLIntensity, 0.0f, 5.0f, "%.1f");
        }
        else
        {
            ImGui::ColorEdit3("Ambient Color", glm::value_ptr(env.Lighting_AmbientColor));
            ImGui::SliderFloat("Ambient Intensity", &env.Lighting_AmbientIntensity, 0.0f, 5.0f, "%.1f");
        }
    }

    if (ImGui::CollapsingHeader("SSAO"))
    {
        ImGui::Checkbox("Use SSAO", &env.SSAO);
        ImGui::SliderFloat("Intensity", &env.SSAO_Intensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Radius", &env.SSAO_Radius, 0.0f, 10.0f);
        ImGui::SliderFloat("Bias", &env.SSAO_Bias, 0.0f, 0.1f);
    }

    ImGui::Checkbox("SSR", &env.SSR);

    if (ImGui::CollapsingHeader("Bloom"))
    {
        ImGui::Checkbox("Use Bloom", &env.Bloom);
        ImGui::SliderFloat("Intensity", &env.Bloom_Intensity, 0.0f, 1.0f);
        ImGui::SliderFloat("Treshold", &env.Bloom_Treshold, 0.0f, 1.0f);
        ImGui::SliderFloat("Knee", &env.Bloom_Knee, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Post Process"))
    {
        ImGui::SliderFloat("Exposure", &env.PostProcess_Exposure, 0.0f, 10.0f);
        ImGui::SliderFloat("Chromatic Aberration", &env.PostProcess_ChromaticStrength, 0.001f, 0.1f);
        ImGui::SliderFloat("Vignette Intensity", &env.PostProcess_VignetteIntensity, 0.0f, 1.0f);
        ImGui::SliderFloat("Vignette Smoothness", &env.PostProcess_VignetteSmoothness, 0.1f, 1.0f);
    }

    ImGui::End();
}