#pragma once
#include "Asset/Types/MaterialAsset.h"
#include "Asset/AssetRegistry.h"
#include <filesystem>
#include <fstream>
#include "Core/Ref.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace rv {

class MaterialSerializer
{
public:
    static Ref<MaterialAsset> CreateNew(
        const std::filesystem::path& path,
        AssetRegistry& registry)
    {
        AssetMeta meta = registry.GetOrCreateMeta(path);
        auto asset = CreateRef<MaterialAsset>(meta.uuid);

        Save(asset, path);
        registry.SaveMeta(path, meta);
        return asset;
    }

    static bool Save(
        const Ref<MaterialAsset>& asset,
        const std::filesystem::path& path)
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

        j["emissiveColor"] = { asset->emissiveColor.r, asset->emissiveColor.g, asset->emissiveColor.b };
        j["emissiveIntensity"] = asset->emissiveIntensity;
        j["specular"] = asset->specular;
        j["ao"] = asset->ao;
        j["normalScale"] = asset->normalScale;
        j["heightScale"] = asset->heightScale;
        j["UVScale"] = { asset->UVScale.x, asset->UVScale.y };
        j["UVOffset"] = { asset->UVOffset.x, asset->UVOffset.y };

        std::ofstream file(path);
        if (!file)
        {
            LOG_ERROR("Cannot write: {}", path.string());
            return false;
        }

        file << j.dump(4);
        return true;
    }

    static bool Load(
        const Ref<MaterialAsset>& asset,
        const std::filesystem::path& path)
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

        if (j.contains("emissiveColor"))
        {
            auto em = j["emissiveColor"];
            asset->emissiveColor = glm::vec3(em[0], em[1], em[2]);
        }
        asset->emissiveIntensity = j.value("emissiveIntensity", 0.0f);
        asset->specular = j.value("specular", 1.0f);
        asset->ao = j.value("ao", 1.0f);
        asset->normalScale = j.value("normalScale", 1.0f);
        asset->heightScale = j.value("heightScale", 0.1f);

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

        return true;
    }
};

}