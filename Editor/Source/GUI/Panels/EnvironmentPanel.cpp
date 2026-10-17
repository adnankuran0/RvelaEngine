#include "EnvironmentPanel.h"
#include "Core/Engine.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "glm/glm.hpp"
#include "EditorUtils.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetRegistry.h"
#include <filesystem>

using namespace rv;

void EnvironmentPanel::Draw(Engine* engine)
{
    ImGui::Begin("Environment");

    auto& env = engine->GetActiveScene().GetEnvironment();

    if (ImGui::CollapsingHeader("Lighting"))
    {
        std::string hdrDisplayName = "Skybox HDR (None)";
        AssetUUID currentUUID = env.GetSkybox().GetHDRUUID();
        if (currentUUID.IsValid())
        {
            std::string fileName = EditorUtils::GetAssetFileName(currentUUID);
            if (!fileName.empty())
            {
                hdrDisplayName = "HDR: " + fileName;
            }
            else if (env.GetSkybox().GetPath().IsValid())
            {
                std::string fn = env.GetSkybox().GetPath().GetFilename();
                std::string stem = std::filesystem::path(fn).stem().string();
                if (!AssetUUID::FromString(stem).IsValid())
                    hdrDisplayName = "HDR: " + fn;
                else
                    hdrDisplayName = "HDR: Skybox";
            }
            else
            {
                hdrDisplayName = "HDR: Skybox";
            }
        }
        else if (env.GetSkybox().GetPath().IsValid())
        {
            std::string fn = env.GetSkybox().GetPath().GetFilename();
            std::string stem = std::filesystem::path(fn).stem().string();
            if (!AssetUUID::FromString(stem).IsValid())
                hdrDisplayName = "HDR: " + fn;
            else
                hdrDisplayName = "HDR: Skybox";
        }

        bool hasHDR = currentUUID.IsValid() || env.GetSkybox().GetPath().IsValid();
        float clearBtnWidth = 24.0f;
        float slotWidth = hasHDR ? (ImGui::GetContentRegionAvail().x - clearBtnWidth - 6.0f) : -FLT_MIN;

        ImGui::Button(hdrDisplayName.c_str(), ImVec2(slotWidth, 24.0f));
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                std::string pathStr((const char*)payload->Data);
                AssetUUID uuid = AssetUUID::Invalid();

                if (pathStr.ends_with(".rtex"))
                {
                    uuid = AssetUUID::FromString(std::filesystem::path(pathStr).stem().string());
                }
                else if (pathStr.ends_with(".hdr"))
                {
                    uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                    if (!uuid.IsValid())
                        uuid = AssetManager::Get().GetRegistry().GetUUID(pathStr);
                }

                if (uuid.IsValid())
                {
                    env.GetSkybox().InitHDR(uuid);
                    engine->GetActiveScene().SetDirty(true);
                }
                else if (pathStr.ends_with(".hdr"))
                {
                    Path absolutePath = Path::FromAbsolute(pathStr);
                    env.GetSkybox().InitHDR(absolutePath);
                    engine->GetActiveScene().SetDirty(true);
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (hasHDR)
        {
            ImGui::SameLine();
            if (ImGui::Button("X##hdrClear", ImVec2(clearBtnWidth, 24.0f)))
            {
                env.GetSkybox().ClearHDR();
                engine->GetActiveScene().SetDirty(true);
            }
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
        ImGui::SliderFloat("Intensity##SSAO", &env.SSAO_Intensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Radius##SSAO", &env.SSAO_Radius, 0.0f, 10.0f);
        ImGui::SliderFloat("Bias##SSAO", &env.SSAO_Bias, 0.0f, 0.1f);
    }

    if (ImGui::CollapsingHeader("Contact Shadows"))
    {
        ImGui::Checkbox("Use Contact Shadows", &env.ContactShadows);
        if (env.ContactShadows)
        {
            ImGui::SliderFloat("Distance##ContactShadows", &env.ContactShadows_Distance, 0.01f, 2.0f, "%.2f m");
            ImGui::SliderFloat("Thickness##ContactShadows", &env.ContactShadows_Thickness, 0.001f, 0.5f, "%.3f m");
            ImGui::SliderFloat("Bias##ContactShadows", &env.ContactShadows_Bias, 0.0001f, 0.05f, "%.4f");
            ImGui::SliderFloat("Intensity##ContactShadows", &env.ContactShadows_Intensity, 0.0f, 2.0f, "%.2f");
            ImGui::SliderInt("Steps##ContactShadows", &env.ContactShadows_Steps, 4, 64);
        }
    }

    if (ImGui::CollapsingHeader("SSR"))
    {
        ImGui::Checkbox("Use SSR", &env.SSR);
        if (env.SSR)
        {
            ImGui::SliderInt("Max Steps", &env.SSR_MaxSteps, 8, 64);
            ImGui::SliderFloat("Max Distance", &env.SSR_MaxDistance, 1.0f, 60.0f, "%.1f m");
            ImGui::SliderFloat("Thickness", &env.SSR_Thickness, 0.05f, 2.0f, "%.2f m");
            ImGui::SliderFloat("Edge Fade", &env.SSR_EdgeFade, 0.01f, 0.3f, "%.2f");
            ImGui::SliderFloat("Max Roughness", &env.SSR_MaxRoughness, 0.1f, 1.0f, "%.2f");
        }
    }

    if (ImGui::CollapsingHeader("SSGI"))
    {
        ImGui::Checkbox("Use SSGI", &env.SSGI);
        if (env.SSGI)
        {
            ImGui::SliderFloat("Intensity##SSGI", &env.SSGI_Intensity, 0.0f, 3.0f, "%.2f");
            ImGui::SliderFloat("Radius##SSGI", &env.SSGI_Radius, 0.5f, 10.0f, "%.1f m");
            ImGui::SliderInt("Steps##SSGI", &env.SSGI_Steps, 4, 32);
        }
    }

    if (ImGui::CollapsingHeader("Bloom"))
    {
        ImGui::Checkbox("Use Bloom", &env.Bloom);
        ImGui::SliderFloat("Intensity##Bloom", &env.Bloom_Intensity, 0.0f, 1.0f);
        ImGui::SliderFloat("Treshold##Bloom", &env.Bloom_Treshold, 0.0f, 1.0f);
        ImGui::SliderFloat("Knee##Bloom", &env.Bloom_Knee, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Fog"))
    {
        ImGui::Checkbox("Enable Fog", &env.Fog_Enabled);
        if (env.Fog_Enabled)
        {
            ImGui::ColorEdit3("Fog Color", glm::value_ptr(env.Fog_Color));
            
            const char* fogModes[] = { "Linear", "Exponential", "Exponential Squared" };
            ImGui::Combo("Mode", &env.Fog_Mode, fogModes, IM_ARRAYSIZE(fogModes));

            if (env.Fog_Mode == 0) // Linear
            {
                ImGui::DragFloat("Start Distance", &env.Fog_Start, 0.5f, 0.0f, 500.0f, "%.1f m");
                ImGui::DragFloat("End Distance", &env.Fog_End, 0.5f, 1.0f, 1000.0f, "%.1f m");
            }
            else // Exp / Exp2
            {
                ImGui::DragFloat("Density", &env.Fog_Density, 0.001f, 0.0001f, 0.2f, "%.4f");
                ImGui::DragFloat("Start Distance", &env.Fog_Start, 0.5f, 0.0f, 200.0f, "%.1f m");
            }
        }
    }

    if (ImGui::CollapsingHeader("Post Process"))
    {
        ImGui::SliderFloat("Exposure", &env.PostProcess_Exposure, 0.0f, 10.0f);
        ImGui::SliderFloat("Brightness", &env.PostProcess_Brightness, -1.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Contrast", &env.PostProcess_Contrast, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Saturation", &env.PostProcess_Saturation, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Chromatic Aberration", &env.PostProcess_ChromaticStrength, 0.001f, 0.1f);
        ImGui::SliderFloat("Vignette Intensity", &env.PostProcess_VignetteIntensity, 0.0f, 1.0f);
        ImGui::SliderFloat("Vignette Smoothness", &env.PostProcess_VignetteSmoothness, 0.1f, 1.0f);
    }

    if ((ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) && GImGui && GImGui->ActiveIdHasBeenEditedThisFrame)
    {
        engine->GetActiveScene().SetDirty(true);
    }

    ImGui::End();
}