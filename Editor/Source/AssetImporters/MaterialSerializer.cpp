#include "MaterialSerializer.h"
#include <Core/Log.h>

using namespace rv;

Ref<MaterialAsset> MaterialSerializer::CreateNew(const std::filesystem::path& path, AssetRegistry& registry)
{
    AssetMeta meta = registry.GetOrCreateMeta(path);
    auto asset = CreateRef<MaterialAsset>(meta.uuid);

    Save(asset, path);
    registry.SaveMeta(path, meta);
    return asset;
}

bool MaterialSerializer::Save(const Ref<MaterialAsset>& asset, const std::filesystem::path& path)
{
    json j;

    j["shadingMode"] = static_cast<int>(asset->shadingMode);
    j["receiveShadows"] = asset->receiveShadows;

    j["transparencyMode"] = static_cast<int>(asset->transparencyMode);
    j["blendMode"] = static_cast<int>(asset->blendMode);
    j["cullMode"] = static_cast<int>(asset->cullMode);
    j["alphaCutoff"] = asset->alphaCutoff;

    j["billboardMode"] = asset->billboardMode;

    j["useAlbedoMap"] = asset->useAlbedoMap;
    if (asset->useAlbedoMap)
        j["albedoTexture"] = asset->albedoTextureUUID.ToString();

    j["albedoColor"] = { asset->albedoColor.r, asset->albedoColor.g, asset->albedoColor.b, asset->albedoColor.a };

    j["useNormalMap"] = asset->useNormalMap;
    if (asset->useNormalMap) j["normalTexture"] = asset->normalTextureUUID.ToString();

    j["useMetallicMap"] = asset->useMetallicMap;
    if (asset->useMetallicMap) j["metallicTexture"] = asset->metallicTextureUUID.ToString();
    j["metallic"] = asset->metallic;

    j["useRoughnessMap"] = asset->useRoughnessMap;
    if (asset->useRoughnessMap) j["roughnessTexture"] = asset->roughnessTextureUUID.ToString();
    j["roughness"] = asset->roughness;

    j["useAOMap"] = asset->useAOMap;
    if (asset->useAOMap) j["aoTexture"] = asset->aoTextureUUID.ToString();

    j["useHeightMap"] = asset->useHeightMap;
    if (asset->useHeightMap) j["heightTexture"] = asset->heightTextureUUID.ToString();

    j["useARMMap"] = asset->useARMMap;
    if (asset->useARMMap) j["armTexture"] = asset->armTextureUUID.ToString();

    j["emissiveColor"] = { asset->emissiveColor.r, asset->emissiveColor.g, asset->emissiveColor.b };
    j["emissiveIntensity"] = asset->emissiveIntensity;
    j["specular"] = asset->specular;
    j["ao"] = asset->ao;
    j["normalScale"] = asset->normalScale;
    j["heightScale"] = asset->heightScale;
    j["invertHeight"] = asset->invertHeight;
    j["UVScale"] = { asset->UVScale.x, asset->UVScale.y };
    j["UVOffset"] = { asset->UVOffset.x, asset->UVOffset.y };

    j["hasOutline"] = asset->hasOutline;
    j["outlineColor"] = { asset->outlineColor.r, asset->outlineColor.g, asset->outlineColor.b, asset->outlineColor.a };
    j["outlineWidth"] = asset->outlineWidth;
    j["outlineFixedSize"] = asset->outlineFixedSize;
    j["outlineXRay"] = asset->outlineXRay;

    j["sampler"] = {
        { "filter", static_cast<int>(asset->samplerDesc.filter) },
        { "repeat", static_cast<int>(asset->samplerDesc.repeat) },
        { "aniso", static_cast<int>(asset->samplerDesc.aniso) }
    };

    if (asset->customShaderUUID.IsValid())
    {
        j["customShader"] = asset->customShaderUUID.ToString();
        json uJson = json::object();
        for (const auto& [name, val] : asset->customUniforms)
        {
            json item;
            item["type"] = static_cast<int>(val.type);
            switch (val.type)
            {
            case ShaderUniformType::Float: item["value"] = val.floatVal; break;
            case ShaderUniformType::Int: item["value"] = val.intVal; break;
            case ShaderUniformType::Bool: item["value"] = val.boolVal; break;
            case ShaderUniformType::Vec2: item["value"] = { val.vec2Val.x, val.vec2Val.y }; break;
            case ShaderUniformType::Vec3:
            case ShaderUniformType::Color3: item["value"] = { val.vec3Val.x, val.vec3Val.y, val.vec3Val.z }; break;
            case ShaderUniformType::Vec4:
            case ShaderUniformType::Color4: item["value"] = { val.vec4Val.x, val.vec4Val.y, val.vec4Val.z, val.vec4Val.w }; break;
            case ShaderUniformType::Texture2D: item["value"] = val.textureUUID.ToString(); break;
            }
            uJson[name] = item;
        }
        j["customUniforms"] = uJson;
    }

    std::ofstream file(path);
    if (!file)
    {
        LOG_ERROR("Cannot write: {}", path.string());
        return false;
    }

    file << j.dump(4);
    return true;
}

bool MaterialSerializer::Load(const Ref<MaterialAsset>& asset, const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file)
    {
        LOG_ERROR("Cannot open file for reading: {}", path.string());
        return false;
    }

    json j;
    file >> j;

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

    asset->useAlbedoMap = j.value("useAlbedoMap", false);
    if (j.contains("albedoTexture"))
        asset->albedoTextureUUID = AssetUUID::FromString(j["albedoTexture"].get<std::string>());

    if (j.contains("albedoColor")) {
        auto col = j["albedoColor"];
        asset->albedoColor = glm::vec4(col[0], col[1], col[2], col.size() > 3 ? col[3].get<float>() : 1.0f);
    }

    asset->useNormalMap = j.value("useNormalMap", false);
    if (j.contains("normalTexture"))
        asset->normalTextureUUID = AssetUUID::FromString(j["normalTexture"].get<std::string>());

    asset->useMetallicMap = j.value("useMetallicMap", false);
    if (j.contains("metallicTexture"))
        asset->metallicTextureUUID = AssetUUID::FromString(j["metallicTexture"].get<std::string>());
    asset->metallic = j.value("metallic", 0.0f);

    asset->useRoughnessMap = j.value("useRoughnessMap", false);
    if (j.contains("roughnessTexture"))
        asset->roughnessTextureUUID = AssetUUID::FromString(j["roughnessTexture"].get<std::string>());
    asset->roughness = j.value("roughness", 1.0f);

    asset->useAOMap = j.value("useAOMap", false);
    if (j.contains("aoTexture"))
        asset->aoTextureUUID = AssetUUID::FromString(j["aoTexture"].get<std::string>());

    asset->useHeightMap = j.value("useHeightMap", false);
    if (j.contains("heightTexture"))
        asset->heightTextureUUID = AssetUUID::FromString(j["heightTexture"].get<std::string>());

    asset->useARMMap = j.value("useARMMap", false);
    if (j.contains("armTexture"))
        asset->armTextureUUID = AssetUUID::FromString(j["armTexture"].get<std::string>());

    if (j.contains("emissiveColor"))
    {
        auto em = j["emissiveColor"];
        asset->emissiveColor = glm::vec3(em[0], em[1], em[2]);
    }
    asset->emissiveIntensity = j.value("emissiveIntensity", 0.0f);
    asset->specular = j.value("specular", 1.0f);
    asset->ao = j.value("ao", 1.0f);
    asset->normalScale = j.value("normalScale", 1.0f);
    asset->heightScale = j.value("heightScale", 0.05f);
    asset->invertHeight = j.value("invertHeight", false);

    if (j.contains("UVScale"))
    {
        auto uv = j["UVScale"];
        asset->UVScale = glm::vec2(uv[0], uv[1]);
    }
    if (j.contains("UVOffset"))
    {
        auto uv = j["UVOffset"];
        asset->UVOffset = glm::vec2(uv[0], uv[1]);
    }

    asset->hasOutline = j.value("hasOutline", false);
    if (j.contains("outlineColor"))
    {
        auto col = j["outlineColor"];
        asset->outlineColor = glm::vec4(col[0], col[1], col[2], col.size() > 3 ? col[3].get<float>() : 1.0f);
    }
    asset->outlineWidth = j.value("outlineWidth", 2.0f);
    asset->outlineFixedSize = j.value("outlineFixedSize", true);
    asset->outlineXRay = j.value("outlineXRay", false);

    if (j.contains("sampler") && j["sampler"].is_object())
    {
        const auto& smp = j["sampler"];
        asset->samplerDesc.filter = static_cast<TextureFilter>(smp.value("filter", static_cast<int>(TextureFilter::Default)));
        asset->samplerDesc.repeat = static_cast<TextureWrap>(smp.value("repeat", smp.value("wrap", static_cast<int>(TextureWrap::Default))));
        asset->samplerDesc.aniso = static_cast<TextureAnisotropy>(smp.value("aniso", smp.value("anisotropy", static_cast<int>(TextureAnisotropy::Default))));
    }

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

    return true;
}
