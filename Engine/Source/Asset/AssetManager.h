#pragma once
#include "AssetRegistry.h"
#include "IAssetLoader.h"

namespace rv {

class AssetManager
{
public:
    static AssetManager& Get() { static AssetManager instance; return instance; }

    void Init(AssetRegistry& db);

    AssetRegistry& GetRegistry() { assert(m_Registry);  return *m_Registry; }

    void RegisterLoader(std::unique_ptr<IAssetLoader> loader);

    template<typename T>
    Ref<T> GetAsset(const AssetUUID& uuid)
    {
        return DynamicCast<T>(GetAssetInternal(uuid));
    }

    void Unload(const AssetUUID& uuid);
    void UnloadAll();

private:
    AssetManager() = default;                        
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

private:
    Ref<Asset> GetAssetInternal(const AssetUUID& uuid);
    IAssetLoader* FindLoader(const std::string& ext) const;

    AssetRegistry* m_Registry = nullptr;
    tsl::robin_map<AssetUUID, WeakRef<Asset>> m_LoadedAssets;
    std::vector<std::unique_ptr<IAssetLoader>> m_Loaders;
};

}