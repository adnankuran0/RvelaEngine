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
#include "Asset/Loaders/AnimationLibraryLoader.h"
#include "Asset/Loaders/SkeletalMeshLoader.h"
#include "Asset/Loaders/SkeletonLoader.h"
#include "Asset/Loaders/ShaderLoader.h"
#include "Utils/ProjectManager.h"

using namespace rv;

void AssetManager::Init(AssetRegistry& registry)
{
    m_Registry = &registry;
    RegisterLoader(std::make_unique<PrefabLoader>());
    RegisterLoader(std::make_unique<MaterialLoader>());
    RegisterLoader(std::make_unique<ShaderLoader>());
    RegisterLoader(std::make_unique<MeshLoader>());
    RegisterLoader(std::make_unique<SceneLoader>());
    RegisterLoader(std::make_unique<TextureLoader>());
    RegisterLoader(std::make_unique<ScriptLoader>());
    RegisterLoader(std::make_unique<AudioClipLoader>());
    RegisterLoader(std::make_unique<AnimationLibraryLoader>());
    RegisterLoader(std::make_unique<SkeletalMeshLoader>());
    RegisterLoader(std::make_unique<SkeletonLoader>());
    if (ProjectManager::IsProjectLoaded())
    {
        m_Registry->Scan(ProjectManager::GetAssetDirectory());
    }
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
        auto assetDir = m_Registry->GetAssetDir();
        if (assetDir.empty() && ProjectManager::IsProjectLoaded())
            assetDir = ProjectManager::GetAssetDirectory();

        if (!assetDir.empty() && std::filesystem::exists(assetDir))
        {
            m_Registry->Scan(assetDir);
        }
    }

    if (!m_Registry->Exists(uuid))
    {
        LOG_ERROR("UUID not found: {}", uuid.ToString()); 
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
        return;
    }
    m_LoadedAssets.erase(it);
}

void AssetManager::Reload(const AssetUUID& uuid)
{
    if (!m_Registry || !m_Registry->Exists(uuid)) return;

    auto it = m_LoadedAssets.find(uuid);
    if (it == m_LoadedAssets.end()) return;

    auto existingAsset = it->second.Lock();
    if (!existingAsset)
    {
        m_LoadedAssets.erase(it);
        return;
    }

    auto path = m_Registry->GetPath(uuid);
    if (path.empty()) return;

    auto ext = path.extension().string();
    IAssetLoader* loader = FindLoader(ext);
    if (!loader) return;

    auto meta = m_Registry->GetMeta(uuid);
    Ref<Asset> reloaded = loader->Load(path, meta);
    if (!reloaded)
    {
        LOG_ERROR("Failed to reload asset: {}", path.string());
        return;
    }

    if (auto texAsset = DynamicCast<TextureAsset>(existingAsset))
    {
        if (auto newTexAsset = DynamicCast<TextureAsset>(reloaded))
        {
            texAsset->m_Width = newTexAsset->m_Width;
            texAsset->m_Height = newTexAsset->m_Height;
            texAsset->m_MipCount = newTexAsset->m_MipCount;
            texAsset->m_Format = newTexAsset->m_Format;
            texAsset->m_SRGB = newTexAsset->m_SRGB;
            texAsset->m_Pixels = std::move(newTexAsset->m_Pixels);
        }
    }
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
