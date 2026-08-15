#include "rvelapch.h"
#include "AssetManager.h"
#include "Core/Log.h"
#include "Asset/Loaders/PrefabLoader.h"
#include "Asset/Loaders/MaterialLoader.h"
#include "Asset/Loaders/MeshLoader.h"
#include "Asset/Loaders/SceneLoader.h"
#include "Asset/Loaders/TextureLoader.h"
#include "Asset/Loaders/ScriptLoader.h"
#include "Asset/Loaders/AudioClipLoader.h"
#include "Utils/ProjectManager.h"

using namespace rv;

void AssetManager::Init(AssetRegistry& registry)
{
    m_Registry = &registry;
    RegisterLoader(std::make_unique<PrefabLoader>());
    RegisterLoader(std::make_unique<MaterialLoader>());
    RegisterLoader(std::make_unique<MeshLoader>());
    RegisterLoader(std::make_unique<SceneLoader>());
    RegisterLoader(std::make_unique<TextureLoader>());
    RegisterLoader(std::make_unique<ScriptLoader>());
    RegisterLoader(std::make_unique<ScriptLoader>());
    RegisterLoader(std::make_unique<AudioClipLoader>());
    m_Registry->Scan(ProjectManager::GetProjectPath() / "Assets");
}

void AssetManager::RegisterLoader(std::unique_ptr<IAssetLoader> loader)
{
    m_Loaders.push_back(std::move(loader));
}

Ref<Asset> AssetManager::GetAssetInternal(const AssetUUID& uuid)
{
    if (!m_Registry) return nullptr;

    auto it = m_LoadedAssets.find(uuid);
    if (it != m_LoadedAssets.end())
        if (auto asset = it->second.Lock()) return asset;
        else m_LoadedAssets.erase(it);

    if (!m_Registry->Exists(uuid))
    {
        // LOG_ERROR("UUID not found: {}", uuid.ToString()); TODO: DISABLED TO SUSPRESS DEFAULT MATERIAL UUID ERRORS
        return nullptr;
    }

    auto path = m_Registry->GetPath(uuid);
    if (path.empty())
    {
        LOG_ERROR("No path for UUID: {}", uuid.ToString());
        return nullptr;
    }

    auto ext = path.extension().string();
    IAssetLoader* loader = FindLoader(ext);
    if (!loader)
    {
        LOG_ERROR("No loader for extension '{}' (uuid: {})", ext, uuid.ToString());
        return nullptr;
    }

    auto meta = m_Registry->GetMeta(uuid);
    Ref<Asset> asset = loader->Load(path, meta);
    if (asset)
        m_LoadedAssets[uuid] = WeakRef<Asset>(asset);

    return asset;
}

void AssetManager::Unload(const AssetUUID& uuid)
{
    auto it = m_LoadedAssets.find(uuid);
    if (it == m_LoadedAssets.end())
    {
        LOG_WARN("Tried to unload asset that isnt loaded: {}", uuid.ToString());
        return;
    }
    m_LoadedAssets.erase(it);
}

void AssetManager::UnloadAll()
{
    m_LoadedAssets.clear();
}

IAssetLoader* AssetManager::FindLoader(const std::string& ext) const
{
    for (auto& loader : m_Loaders)
        for (auto& supported : loader->GetSupportedExtensions())
            if (supported == ext)
                return loader.get();
    return nullptr;
}
