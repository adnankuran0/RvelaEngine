#include "rvelapch.h"
#include "ScriptLoader.h"
#include "Asset/Types/ScriptAsset.h"
#include "Asset/AssetManager.h"
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

Ref<Asset> ScriptLoader::LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta)
{
    if (!data || size == 0)
    {
        LOG_ERROR("Empty script buffer for asset: {}", meta.uuid.ToString());
        return nullptr;
    }

    std::string source(reinterpret_cast<const char*>(data), size);
    auto asset = CreateRef<ScriptAsset>(meta.uuid);
    asset->m_Source = std::move(source);

    auto path = AssetManager::Get().GetRegistry().GetPath(meta.uuid);
    if (!path.empty())
        asset->m_ScriptName = path.stem().string();
    else
        asset->m_ScriptName = "Script_" + meta.uuid.ToString();

    return asset;
}
