#include "rvelapch.h"
#include "SceneLoader.h"
#include "Asset/Types/SceneAsset.h"
#include "Core/Log.h"
#include <fstream>
#include <sstream>
#include "Asset/AssetMeta.h"

using namespace rv;

Ref<Asset> SceneLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
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
        LOG_ERROR("Empty scene file: {}", assetPath.string());
        return nullptr;
    }

    auto asset = CreateRef<SceneAsset>(meta.uuid);
    asset->m_JSON = std::move(json);

    return asset;
}