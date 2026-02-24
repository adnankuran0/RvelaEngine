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

    j["Bloom"] = Bloom;
    j["Bloom_Treshold"] = Bloom_Treshold;
    j["Bloom_Knee"] = Bloom_Knee;

    j["PostProcess_Exposure"] = PostProcess_Exposure;
    j["PostProcess_VignetteIntensity"] = PostProcess_VignetteIntensity;
    j["PostProcess_VignetteSmoothness"] = PostProcess_VignetteSmoothness;
    j["PostProcess_ChromaticStrength"] = PostProcess_ChromaticStrength;

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

    if (j.contains("Bloom"))
        Bloom = j["Bloom"];

    if (j.contains("Bloom_Treshold"))
        Bloom_Treshold = j["Bloom_Treshold"];

    if (j.contains("Bloom_Knee"))
        Bloom_Knee = j["Bloom_Knee"];

    if (j.contains("PostProcess_Exposure"))
        PostProcess_Exposure = j["PostProcess_Exposure"];

    if (j.contains("PostProcess_VignetteIntensity"))
        PostProcess_VignetteIntensity = j["PostProcess_VignetteIntensity"];

    if (j.contains("PostProcess_VignetteSmoothness"))
        PostProcess_VignetteSmoothness = j["PostProcess_VignetteSmoothness"];

    if (j.contains("PostProcess_ChromaticStrength"))
        PostProcess_ChromaticStrength = j["PostProcess_ChromaticStrength"];

    if (j.contains("HDRPath"))
        m_Skybox.InitHDR(Path::FromVirtual(j["HDRPath"]));
}