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

    return asset;
}