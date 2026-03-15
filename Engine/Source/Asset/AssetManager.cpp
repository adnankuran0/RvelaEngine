#include "rvelapch.h"
#include "AssetManager.h"
#include "Core/Log.h"

using namespace rv;

void AssetManager::Init(AssetRegistry& registry)
{
    m_Registry = &registry;
}

void AssetManager::RegisterLoader(std::unique_ptr<IAssetLoader> loader)
{
    m_Loaders.push_back(std::move(loader));
}

Ref<Asset> AssetManager::GetAssetInternal(const AssetUUID& uuid)
{
    if (!m_Registry)
    {
        LOG_ERROR("AssetManager not initialized!");
        return nullptr;
    }

    
    auto cacheIt = m_LoadedAssets.find(uuid);
    if (cacheIt != m_LoadedAssets.end())
    {
        Ref<Asset> asset = cacheIt->second.Lock();
        if (asset) return asset;

        // Weak ref expired
        m_LoadedAssets.erase(cacheIt);
    }

    if (!m_Registry->Exists(uuid))
    {
        LOG_ERROR("UUID not found in registry: {}", uuid.ToString());
        return nullptr;
    }

    auto path = m_Registry->GetPath(uuid);
    auto meta = m_Registry->GetMeta(uuid);
    auto ext = path.extension().string();

    IAssetLoader* loader = FindLoader(ext);
    if (!loader)
    {
        LOG_ERROR("No loader registered for extension: {}", ext);
        return nullptr;
    }

    Ref<Asset> asset = loader->Load(path, meta);
    if (!asset)
    {
        LOG_ERROR("Loader failed for: {}", path.string());
        return nullptr;
    }

    m_LoadedAssets[uuid] = WeakRef<Asset>(asset);
    return asset;
}

void AssetManager::Unload(const AssetUUID& uuid)
{
    auto it = m_LoadedAssets.find(uuid);
    if (it == m_LoadedAssets.end())
    {
        LOG_WARN("Tried to unload asset that isn't loaded: {}", uuid.ToString());
        return;
    }
    m_LoadedAssets.erase(it);
}

void AssetManager::UnloadAll()
{
    m_LoadedAssets.clear();
    LOG_INFO("All assets unloaded.");
}

IAssetLoader* AssetManager::FindLoader(const std::string& ext) const
{
    for (auto& loader : m_Loaders)
        for (auto& supported : loader->GetSupportedExtensions())
            if (supported == ext)
                return loader.get();
    return nullptr;
}
