#include "rvelapch.h"
#include "PrefabLoader.h"
#include "Asset/Types/PrefabAsset.h"
#include "Core/Log.h"
#include <fstream>
#include <sstream>
#include "Asset/AssetMeta.h"

using namespace rv;

Ref<Asset> PrefabLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath);
    if (!file)
    {
        LOG_ERROR("Cannot open: {}", assetPath.string());
        return nullptr;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();

    if (json.empty())
    {
        LOG_ERROR("Empty prefab file: {}", assetPath.string());
        return nullptr;
    }

    auto asset = CreateRef<PrefabAsset>(meta.uuid);
    asset->m_JSON = std::move(json);
    return asset;
}

Ref<Asset> PrefabLoader::LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta)
{
    if (!data || size == 0)
    {
        LOG_ERROR("Empty prefab buffer for asset: {}", meta.uuid.ToString());
        return nullptr;
    }

    auto asset = CreateRef<PrefabAsset>(meta.uuid);
    asset->m_JSON.assign(reinterpret_cast<const char*>(data), size);
    return asset;
}
