#pragma once
#include <unordered_map>
#include <filesystem>
#include "AssetUUID.h"
#include "Asset.h"
#include "AssetMeta.h"
#include "TextureAsset.h"

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
        Ref<Asset> asset = LoadAssetFromFile(path);
        if (asset)
            s_LoadedAssets[uuid] = WeakRef<Asset>(asset);

        return DynamicCast<T>(asset);
    }

private:
    void ScanAssets(const std::filesystem::path& dir);

    inline static AssetHeader ReadHeader(std::ifstream& inFile, uint32_t expectedMagic);
    static Ref<Asset> LoadAssetFromFile(const std::filesystem::path& path);

    template <typename T>
    inline static std::unique_ptr<T> ReadMeta(std::ifstream& inFile, const AssetHeader& header)
    {
        static_assert(std::is_base_of<AssetMeta, T>::value, "T must derive from AssetMeta");

        std::vector<char> metaBuffer(header.metaSize);
        inFile.read(metaBuffer.data(), header.metaSize);
        if (!inFile) 
        {
            LOG_ERROR("Failed to read metadata");
            return nullptr;
        }

        std::unique_ptr<T> meta = std::make_unique<T>();

        size_t offset = 0;
        meta->Deserialize(metaBuffer, offset); 

        return meta;
    }

private:
    inline static std::filesystem::path s_AssetDirectory;
    inline static std::unordered_map<AssetUUID, std::filesystem::path> s_UUIDToPath;
    inline static std::unordered_map<AssetUUID, WeakRef<Asset>> s_LoadedAssets;
};
