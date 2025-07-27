#pragma once
#include <filesystem>
#include "AssetUUID.h"
#include "Asset.h"
#include "AssetMeta.h"
#include "TextureAsset.h"
#include "tsl/robin_map.h"
#include "Assets/AssetLoader.h"

class AssetRegistry 
{
public:
    void Init(const std::filesystem::path& assetDir);
    static std::filesystem::path GetAssetPath(const AssetUUID& uuid);

    static inline bool IsExist(const AssetUUID& uuid)
    {
        return s_UUIDToPath.contains(uuid);
    }

    static inline bool IsLoaded(const AssetUUID& uuid)
    {
        return s_LoadedAssets.contains(uuid);
    }



    template<typename T>
    inline static Ref<T> GetAsset(AssetUUID uuid)
    {
        if (!IsExist(uuid))
            return nullptr;

        if (IsLoaded(uuid))
        {
            Ref<Asset> asset = s_LoadedAssets[uuid].Lock();
            if (asset)
                return DynamicCast<T>(asset);
        }
        
        //If asset exists but not loaded yet then we are doing lazy loading
        auto path = s_UUIDToPath[uuid];
        Ref<Asset> asset = AssetLoader::Load(path);
        if (asset)
            s_LoadedAssets[uuid] = WeakRef<Asset>(asset);

        return DynamicCast<T>(asset);
    }

private:
    void ScanAssets(const std::filesystem::path& dir);

private:
    inline static std::filesystem::path s_AssetDirectory;
    inline static tsl::robin_map<AssetUUID, std::filesystem::path> s_UUIDToPath;
    inline static tsl::robin_map<AssetUUID, WeakRef<Asset>> s_LoadedAssets;
};
