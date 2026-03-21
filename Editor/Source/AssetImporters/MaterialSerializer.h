#pragma once
#include "Asset/Types/MaterialAsset.h"
#include "Asset/AssetRegistry.h"
#include <filesystem>
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

        if (asset->useAlbedoMap)
            j["albedoTexture"] = asset->albedoTextureUUID.ToString();
        else
            j["albedoColor"] = { asset->albedoColor.r, asset->albedoColor.g, asset->albedoColor.b };

        if (asset->useNormalMap) j["normalTexture"] = asset->normalTextureUUID.ToString();
        if (asset->useMetallicMap) j["metallicTexture"] = asset->metallicTextureUUID.ToString();
        j["metallic"] = asset->metallic;
        if (asset->useRoughnessMap) j["roughnessTexture"] = asset->roughnessTextureUUID.ToString();
        j["roughness"] = asset->roughness;
        if (asset->useAOMap) j["aoTexture"] = asset->aoTextureUUID.ToString();
        if (asset->useHeightMap) j["heightTexture"] = asset->heightTextureUUID.ToString();

        j["emissiveColor"] = { asset->emissiveColor.r, asset->emissiveColor.g, asset->emissiveColor.b };
        j["emissiveIntensity"] = asset->emissiveIntensity;
        j["specular"] = asset->specular;
        j["ao"] = asset->ao;
        j["normalScale"] = asset->normalScale;
        j["heightScale"] = asset->heightScale;
        j["UVScale"] = { asset->UVScale.x,  asset->UVScale.y };
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
};

}