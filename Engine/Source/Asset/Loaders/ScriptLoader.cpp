#include "rvelapch.h"
#include "ScriptLoader.h"
#include "Asset/Types/ScriptAsset.h"
#include "Core/Log.h"
#include <fstream>
#include <sstream>
#include "Asset/AssetMeta.h"

using namespace rv;

Ref<Asset> ScriptLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath);
    if (!file)
    {
        LOG_ERROR("Cannot open script: {}", assetPath.string());
        return nullptr;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    if (source.empty())
    {
        LOG_ERROR("Empty script file: {}", assetPath.string());
        return nullptr;
    }

    auto asset = CreateRef<ScriptAsset>(meta.uuid);
    asset->m_Source = std::move(source);
    asset->m_ScriptName = assetPath.stem().string();
    return asset;
}