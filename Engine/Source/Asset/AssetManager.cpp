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
#include "Asset/Types/TextureAsset.h"
#include "Asset/Types/MeshAsset.h"
#include "Asset/Types/SkeletalMeshAsset.h"
#include "Asset/Types/MaterialAsset.h"
#include "Asset/Types/ShaderAsset.h"
#include "Asset/Types/AudioClipAsset.h"
#include "Renderer/TextureCache.h"
#include "Renderer/MeshCache.h"
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
    std::unique_lock<std::shared_mutex> lock(m_AssetMutex);
    m_Loaders.push_back(std::move(loader));
}

Ref<Asset> AssetManager::GetAssetInternal(const AssetUUID& uuid)
{
    if (!m_Registry) return nullptr;

    {
        std::shared_lock<std::shared_mutex> readLock(m_AssetMutex);
        auto it = m_LoadedAssets.find(uuid);
        if (it != m_LoadedAssets.end())
        {
            if (auto asset = it->second.Lock())
                return asset;
        }
    }

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

    {
        std::unique_lock<std::shared_mutex> writeLock(m_AssetMutex);
        auto it = m_LoadedAssets.find(uuid);
        if (it != m_LoadedAssets.end())
        {
            if (auto existing = it->second.Lock())
                return existing;
        }

        if (asset)
            m_LoadedAssets[uuid] = WeakRef<Asset>(asset);
        else if (it != m_LoadedAssets.end())
            m_LoadedAssets.erase(it);
    }

    return asset;
}

void AssetManager::Unload(const AssetUUID& uuid)
{
    {
        std::unique_lock<std::shared_mutex> lock(m_AssetMutex);
        auto it = m_LoadedAssets.find(uuid);
        if (it == m_LoadedAssets.end())
        {
            return;
        }
        m_LoadedAssets.erase(it);
    }
    TextureCache::Get().Remove(uuid);
    MeshCache::Get().Remove(uuid);
}

void AssetManager::Reload(const AssetUUID& uuid)
{
    Ref<Asset> existingAsset = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock(m_AssetMutex);
        if (!m_Registry || !m_Registry->Exists(uuid)) return;

        auto it = m_LoadedAssets.find(uuid);
        if (it == m_LoadedAssets.end()) return;

        existingAsset = it->second.Lock();
    }

    if (!existingAsset)
    {
        std::unique_lock<std::shared_mutex> lock(m_AssetMutex);
        m_LoadedAssets.erase(uuid);
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
            TextureCache::Get().Remove(uuid);
        }
    }
    else if (auto meshAsset = DynamicCast<MeshAsset>(existingAsset))
    {
        if (auto newMeshAsset = DynamicCast<MeshAsset>(reloaded))
        {
            meshAsset->CopyFrom(*newMeshAsset);
            MeshCache::Get().Remove(uuid);
        }
    }
    else if (auto skelMeshAsset = DynamicCast<SkeletalMeshAsset>(existingAsset))
    {
        if (auto newSkelMeshAsset = DynamicCast<SkeletalMeshAsset>(reloaded))
        {
            skelMeshAsset->CopyFrom(*newSkelMeshAsset);
            MeshCache::Get().Remove(uuid);
        }
    }
    else if (auto matAsset = DynamicCast<MaterialAsset>(existingAsset))
    {
        if (auto newMatAsset = DynamicCast<MaterialAsset>(reloaded))
        {
            *matAsset = *newMatAsset;
        }
    }
    else if (auto shaderAsset = DynamicCast<ShaderAsset>(existingAsset))
    {
        if (auto newShader = DynamicCast<ShaderAsset>(reloaded))
        {
            shaderAsset->filePath = std::move(newShader->filePath);
            shaderAsset->lastWriteTime = newShader->lastWriteTime;
            shaderAsset->source = std::move(newShader->source);
            shaderAsset->isLit = newShader->isLit;
            shaderAsset->renderModes = newShader->renderModes;
            shaderAsset->uniforms = std::move(newShader->uniforms);
            shaderAsset->samplers = std::move(newShader->samplers);
            shaderAsset->compileError = std::move(newShader->compileError);
            shaderAsset->compileSuccess = newShader->compileSuccess;
            shaderAsset->staticShader = std::move(newShader->staticShader);
            shaderAsset->skeletalShader = std::move(newShader->skeletalShader);
        }
    }
    else if (auto audioAsset = DynamicCast<AudioClipAsset>(existingAsset))
    {
        if (auto newAudio = DynamicCast<AudioClipAsset>(reloaded))
        {
            audioAsset->CopyFrom(*newAudio);
        }
    }
}

void AssetManager::UnloadAll()
{
    std::unique_lock<std::shared_mutex> lock(m_AssetMutex);
    m_LoadedAssets.clear();
}

IAssetLoader* AssetManager::FindLoader(const std::string& ext) const
{
    std::shared_lock<std::shared_mutex> lock(m_AssetMutex);
    for (auto& loader : m_Loaders)
        for (auto& supported : loader->GetSupportedExtensions())
            if (supported == ext)
                return loader.get();
    return nullptr;
}
