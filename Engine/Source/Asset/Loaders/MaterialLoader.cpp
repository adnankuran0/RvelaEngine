#include "rvelapch.h"
#include "MaterialLoader.h"
#include "Asset/Types/MaterialAsset.h"
#include "Core/Log.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "Asset/AssetMeta.h"

using namespace rv;
using json = nlohmann::json;

MaterialLoader::MaterialLoader()
{
    s_DefaultMaterial = CreateRef<MaterialAsset>(AssetUUID::Invalid());
}

Ref<Asset> MaterialLoader::Load(
    const std::filesystem::path& assetPath,
    const AssetMeta& meta)
{
    std::ifstream file(assetPath);
    if (!file)
    {
        LOG_ERROR("Cannot open: {}", assetPath.string());
        return nullptr;
    }

    json j;
    try
    {
        j = json::parse(file);
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("JSON error in {}: {}", assetPath.string(), e.what());
        return nullptr;
    }

    auto asset = CreateRef<MaterialAsset>(meta.uuid);

    if (j.contains("shadingMode"))
        asset->shadingMode = static_cast<ShadingMode>(j["shadingMode"].get<int>());
    asset->receiveShadows = j.value("receiveShadows", true);

    if (j.contains("transparencyMode"))
        asset->transparencyMode = static_cast<TransparencyMode>(j["transparencyMode"].get<int>());
    if (j.contains("blendMode"))
        asset->blendMode = static_cast<BlendMode>(j["blendMode"].get<int>());
    if (j.contains("cullMode"))
        asset->cullMode = static_cast<CullMode>(j["cullMode"].get<int>());
    if (j.contains("alphaCutoff"))
        asset->alphaCutoff = j["alphaCutoff"].get<float>();

    if (j.contains("billboardMode"))
        asset->billboardMode = static_cast<BillboardMode>(j["billboardMode"].get<int>());

    if (j.contains("albedoTexture"))
    {
        asset->albedoTextureUUID = AssetUUID::FromString(j["albedoTexture"]);
        asset->useAlbedoMap = j.value("useAlbedoMap", true);
    }
    if (j.contains("normalTexture"))
    {
        asset->normalTextureUUID = AssetUUID::FromString(j["normalTexture"]);
        asset->useNormalMap = j.value("useNormalMap", true);
    }
    if (j.contains("metallicTexture"))
    {
        asset->metallicTextureUUID = AssetUUID::FromString(j["metallicTexture"]);
        asset->useMetallicMap = j.value("useMetallicMap", true);
    }
    if (j.contains("roughnessTexture"))
    {
        asset->roughnessTextureUUID = AssetUUID::FromString(j["roughnessTexture"]);
        asset->useRoughnessMap = j.value("useRoughnessMap", true);
    }
    if (j.contains("aoTexture"))
    {
        asset->aoTextureUUID = AssetUUID::FromString(j["aoTexture"]);
        asset->useAOMap = j.value("useAOMap", true);
    }
    if (j.contains("heightTexture"))
    {
        asset->heightTextureUUID = AssetUUID::FromString(j["heightTexture"]);
        asset->useHeightMap = j.value("useHeightMap", true);
    }
    if (j.contains("armTexture"))
    {
        asset->armTextureUUID = AssetUUID::FromString(j["armTexture"]);
        asset->useARMMap = j.value("useARMMap", true);
    }

    if (j.contains("albedoColor"))
    {
        auto& col = j["albedoColor"];
        asset->albedoColor = { col[0], col[1], col[2], col.size() > 3 ? col[3].get<float>() : 1.0f };
    }
    if (j.contains("emissiveColor"))
        asset->emissiveColor = { j["emissiveColor"][0], j["emissiveColor"][1], j["emissiveColor"][2] };

    asset->emissiveIntensity = j.value("emissiveIntensity", asset->emissiveIntensity);
    asset->metallic = j.value("metallic", asset->metallic);
    asset->specular = j.value("specular", asset->specular);
    asset->roughness = j.value("roughness", asset->roughness);
    asset->ao = j.value("ao", asset->ao);
    asset->normalScale = j.value("normalScale", asset->normalScale);
    asset->heightScale = j.value("heightScale", asset->heightScale);

    if (j.contains("UVScale"))
        asset->UVScale = { j["UVScale"][0], j["UVScale"][1] };
    if (j.contains("UVOffset"))
        asset->UVOffset = { j["UVOffset"][0], j["UVOffset"][1] };

    asset->useTriplanar = j.value("useTriplanar", false);
    asset->useWorldTriplanar = j.value("useWorldTriplanar", false);
    asset->triplanarSharpness = j.value("triplanarSharpness", 4.0f);

    if (j.contains("customShader"))
    {
        asset->customShaderUUID = AssetUUID::FromString(j["customShader"].get<std::string>());
    }
    if (j.contains("customUniforms") && j["customUniforms"].is_object())
    {
        asset->customUniforms.clear();
        for (auto& [name, item] : j["customUniforms"].items())
        {
            CustomUniformValue val;
            val.type = static_cast<ShaderUniformType>(item.value("type", 0));
            auto& v = item["value"];
            switch (val.type)
            {
            case ShaderUniformType::Float: val.floatVal = v.get<float>(); break;
            case ShaderUniformType::Int: val.intVal = v.get<int>(); break;
            case ShaderUniformType::Bool: val.boolVal = v.get<bool>(); break;
            case ShaderUniformType::Vec2: val.vec2Val = glm::vec2(v[0].get<float>(), v[1].get<float>()); break;
            case ShaderUniformType::Vec3:
            case ShaderUniformType::Color3: val.vec3Val = glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>()); break;
            case ShaderUniformType::Vec4:
            case ShaderUniformType::Color4: val.vec4Val = glm::vec4(v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>()); break;
            case ShaderUniformType::Texture2D: val.textureUUID = AssetUUID::FromString(v.get<std::string>()); break;
            }
            asset->customUniforms[name] = val;
        }
    }

    return asset;
}