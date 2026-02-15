#pragma once
#include <filesystem>
#include "Core/Ref.h"
#include "tsl/robin_map.h"
#include "AssetUUID.h"
#include "AssetLoader.h"

class Asset;
class AssetLoader;

class AssetRegistry 
{
public:
    void Init(const std::filesystem::path& assetDir);
    static std::filesystem::path GetAssetPath(const AssetUUID& uuid);
    inline static std::filesystem::path GetAssetDirectory() { return s_AssetDirectory; }
    static void ScanAssets(const std::filesystem::path& dir = s_AssetDirectory);
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
        if (!m_IsInitialized)
        {
            LOG_ERROR("Tried to get asset while asset registry is not initialized yet!");
            return nullptr;
        }

        //if (!IsExist(uuid))
        //{
        //    LOG_ERROR("Asset UUID is not exist in the asset registry!");
        //    return nullptr;

        //}

        if (IsLoaded(uuid))
        {
            Ref<Asset> asset = s_LoadedAssets[uuid].Lock();
            if (asset)
                return DynamicCast<T>(asset);
            else
            {
                // Asset was deleted, we should remove it from loaded assets so we can reload
                s_LoadedAssets.erase(uuid);
            }
        }
        

        //If asset exists but not loaded yet then we are doing lazy loading
        auto path = s_UUIDToPath[uuid];
        Ref<Asset> asset = AssetLoader::Load(path);
        if (asset)
            s_LoadedAssets[uuid] = WeakRef<Asset>(asset);

        return DynamicCast<T>(asset);
    }

    


private:
    inline static bool m_IsInitialized = false;
    inline static std::filesystem::path s_AssetDirectory;
    inline static tsl::robin_map<AssetUUID, std::filesystem::path> s_UUIDToPath;
    inline static tsl::robin_map<AssetUUID, WeakRef<Asset>> s_LoadedAssets;
};
