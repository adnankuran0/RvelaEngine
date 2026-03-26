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
    AssetUUID uuid{};
    s_DefaultMaterial = CreateRef<MaterialAsset>(uuid);
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

    if (j.contains("albedoTexture")) { asset->albedoTextureUUID = AssetUUID::FromString(j["albedoTexture"]); asset->useAlbedoMap = true; }
    if (j.contains("normalTexture")) { asset->normalTextureUUID = AssetUUID::FromString(j["normalTexture"]); asset->useNormalMap = true; }
    if (j.contains("metallicTexture")) { asset->metallicTextureUUID = AssetUUID::FromString(j["metallicTexture"]); asset->useMetallicMap = true; }
    if (j.contains("roughnessTexture")) { asset->roughnessTextureUUID = AssetUUID::FromString(j["roughnessTexture"]); asset->useRoughnessMap = true; }
    if (j.contains("aoTexture")) { asset->aoTextureUUID = AssetUUID::FromString(j["aoTexture"]); asset->useAOMap = true; }
    if (j.contains("heightTexture")) { asset->heightTextureUUID = AssetUUID::FromString(j["heightTexture"]); asset->useHeightMap = true; }

    if (j.contains("albedoColor")) asset->albedoColor = { j["albedoColor"][0], j["albedoColor"][1], j["albedoColor"][2] };
    if (j.contains("emissiveColor")) asset->emissiveColor = { j["emissiveColor"][0], j["emissiveColor"][1], j["emissiveColor"][2] };
    if (j.contains("emissiveIntensity")) asset->emissiveIntensity = j["emissiveIntensity"];
    if (j.contains("metallic")) asset->metallic = j["metallic"];
    if (j.contains("specular")) asset->specular = j["specular"];
    if (j.contains("roughness")) asset->roughness = j["roughness"];
    if (j.contains("ao")) asset->ao = j["ao"];
    if (j.contains("normalScale")) asset->normalScale = j["normalScale"];
    if (j.contains("heightScale")) asset->heightScale = j["heightScale"];
    if (j.contains("UVScale")) asset->UVScale = { j["UVScale"][0], j["UVScale"][1] };
    if (j.contains("UVOffset")) asset->UVOffset = { j["UVOffset"][0], j["UVOffset"][1] };

    return asset;
}