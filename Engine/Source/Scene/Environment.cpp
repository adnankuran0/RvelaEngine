#include "rvelapch.h"
#include "Environment.h"

using namespace rv;
using json = nlohmann::json;

json Environment::Serialize()
{
    json j;

    j["Lighting_IBL"] = Lighting_IBL;
    j["Lighting_IBLIntensity"] = Lighting_IBLIntensity;
    j["Lighting_AmbientColor"] = {
        Lighting_AmbientColor.x,
        Lighting_AmbientColor.y,
        Lighting_AmbientColor.z
    };
    j["Lighting_AmbientIntensity"] = Lighting_AmbientIntensity;

    j["SSAO"] = SSAO;
    j["SSAO_Radius"] = SSAO_Radius;
    j["SSAO_Bias"] = SSAO_Bias;
    j["SSAO_Intensity"] = SSAO_Intensity;

    j["SSR"] = SSR;
    j["SSR_MaxSteps"] = SSR_MaxSteps;
    j["SSR_MaxDistance"] = SSR_MaxDistance;
    j["SSR_Thickness"] = SSR_Thickness;
    j["SSR_EdgeFade"] = SSR_EdgeFade;
    j["SSR_MaxRoughness"] = SSR_MaxRoughness;

    j["SSGI"] = SSGI;
    j["SSGI_Intensity"] = SSGI_Intensity;
    j["SSGI_Radius"] = SSGI_Radius;
    j["SSGI_Steps"] = SSGI_Steps;

    j["Bloom"] = Bloom;
    j["Bloom_Intensity"] = Bloom_Intensity;
    j["Bloom_Treshold"] = Bloom_Treshold;
    j["Bloom_Knee"] = Bloom_Knee;

    j["PostProcess_Exposure"] = PostProcess_Exposure;
    j["PostProcess_Brightness"] = PostProcess_Brightness;
    j["PostProcess_Contrast"] = PostProcess_Contrast;
    j["PostProcess_Saturation"] = PostProcess_Saturation;
    j["PostProcess_VignetteIntensity"] = PostProcess_VignetteIntensity;
    j["PostProcess_VignetteSmoothness"] = PostProcess_VignetteSmoothness;
    j["PostProcess_ChromaticStrength"] = PostProcess_ChromaticStrength;

    j["Fog_Enabled"] = Fog_Enabled;
    j["Fog_Color"] = { Fog_Color.r, Fog_Color.g, Fog_Color.b };
    j["Fog_Density"] = Fog_Density;
    j["Fog_Start"] = Fog_Start;
    j["Fog_End"] = Fog_End;
    j["Fog_Mode"] = Fog_Mode;

    j["HDR"] = m_Skybox.GetHDRUUID().IsValid() ? m_Skybox.GetHDRUUID().ToString() : "";
    j["HDRPath"] = m_Skybox.GetPath().GetVirtualStr();

    return j;
}

void Environment::Deserialize(const json& j)
{
    if (j.contains("Lighting_IBL"))
        Lighting_IBL = j["Lighting_IBL"];

    if (j.contains("Lighting_IBLIntensity"))
        Lighting_IBLIntensity = j["Lighting_IBLIntensity"];

    if (j.contains("Lighting_AmbientColor"))
    {
        auto& c = j["Lighting_AmbientColor"];
        Lighting_AmbientColor = glm::vec3(c[0], c[1], c[2]);
    }

    if (j.contains("Lighting_AmbientIntensity"))
        Lighting_AmbientIntensity = j["Lighting_AmbientIntensity"];

    if (j.contains("SSAO"))
        SSAO = j["SSAO"];

    if (j.contains("SSAO_Radius"))
        SSAO_Radius = j["SSAO_Radius"];

    if (j.contains("SSAO_Bias"))
        SSAO_Bias = j["SSAO_Bias"];

    if (j.contains("SSAO_Intensity"))
        SSAO_Intensity = j["SSAO_Intensity"];

    if (j.contains("SSR"))
        SSR = j["SSR"];
    if (j.contains("SSR_MaxSteps"))
        SSR_MaxSteps = j["SSR_MaxSteps"];
    if (j.contains("SSR_MaxDistance"))
        SSR_MaxDistance = j["SSR_MaxDistance"];
    if (j.contains("SSR_Thickness"))
        SSR_Thickness = j["SSR_Thickness"];
    if (j.contains("SSR_EdgeFade"))
        SSR_EdgeFade = j["SSR_EdgeFade"];
    if (j.contains("SSR_MaxRoughness"))
        SSR_MaxRoughness = j["SSR_MaxRoughness"];

    if (j.contains("SSGI"))
        SSGI = j["SSGI"];
    if (j.contains("SSGI_Intensity"))
        SSGI_Intensity = j["SSGI_Intensity"];
    if (j.contains("SSGI_Radius"))
        SSGI_Radius = j["SSGI_Radius"];
    if (j.contains("SSGI_Steps"))
        SSGI_Steps = j["SSGI_Steps"];

    if (j.contains("Bloom"))
        Bloom = j["Bloom"];

    if (j.contains("Bloom_Intensity "))
        Bloom_Intensity = j["Bloom_Intensity "];

    if (j.contains("Bloom_Treshold"))
        Bloom_Treshold = j["Bloom_Treshold"];

    if (j.contains("Bloom_Knee"))
        Bloom_Knee = j["Bloom_Knee"];

    if (j.contains("PostProcess_Exposure"))
        PostProcess_Exposure = j["PostProcess_Exposure"];

    if (j.contains("PostProcess_Brightness"))
        PostProcess_Brightness = j["PostProcess_Brightness"];

    if (j.contains("PostProcess_Contrast"))
        PostProcess_Contrast = j["PostProcess_Contrast"];

    if (j.contains("PostProcess_Saturation"))
        PostProcess_Saturation = j["PostProcess_Saturation"];

    if (j.contains("PostProcess_VignetteIntensity"))
        PostProcess_VignetteIntensity = j["PostProcess_VignetteIntensity"];

    if (j.contains("PostProcess_VignetteSmoothness"))
        PostProcess_VignetteSmoothness = j["PostProcess_VignetteSmoothness"];

    if (j.contains("PostProcess_ChromaticStrength"))
        PostProcess_ChromaticStrength = j["PostProcess_ChromaticStrength"];

    if (j.contains("Fog_Enabled"))
        Fog_Enabled = j["Fog_Enabled"];

    if (j.contains("Fog_Color"))
    {
        auto& fc = j["Fog_Color"];
        Fog_Color = glm::vec3(fc[0], fc[1], fc[2]);
    }

    if (j.contains("Fog_Density"))
        Fog_Density = j["Fog_Density"];

    if (j.contains("Fog_Start"))
        Fog_Start = j["Fog_Start"];

    if (j.contains("Fog_End"))
        Fog_End = j["Fog_End"];

    if (j.contains("Fog_Mode"))
        Fog_Mode = j["Fog_Mode"];

    if (j.contains("HDR") && !j["HDR"].get<std::string>().empty())
    {
        AssetUUID uuid = AssetUUID::FromString(j["HDR"].get<std::string>());
        if (uuid.IsValid())
        {
            m_Skybox.InitHDR(uuid);
        }
    }
    else if (j.contains("HDRPath"))
    {
        std::string pathStr = j["HDRPath"].get<std::string>();
        if (!pathStr.empty())
        {
            m_Skybox.InitHDR(Path::FromVirtual(pathStr));
        }
    }
}