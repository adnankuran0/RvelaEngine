#pragma once
#include <filesystem>
#include "tsl/robin_map.h"
#include "AssetUUID.h"
#include "AssetMeta.h"

namespace rv {

class AssetRegistry
{
public:
    void Scan(const std::filesystem::path& assetDir);

    AssetMeta GetMeta(const AssetUUID& uuid) const;
    AssetMeta GetOrCreateMeta(const std::filesystem::path& path);
    void SaveMeta(const std::filesystem::path& path, const AssetMeta& meta);

    bool Exists(const AssetUUID& uuid) const;
    std::filesystem::path GetPath(const AssetUUID& uuid) const;
    const std::filesystem::path& GetAssetDir() const { return m_AssetDir; }
    AssetUUID GetUUID(const std::filesystem::path& path) const;
    std::vector<AssetUUID> GetDependencies(const AssetUUID& uuid) const;
    void RegisterPath(const AssetUUID& uuid, const std::filesystem::path& path);
    void RegisterSubAsset(const AssetUUID& uuid,
        const std::filesystem::path& cachePath,
        const std::string& importerID);
  
private:
    std::filesystem::path m_AssetDir;
    tsl::robin_map<AssetUUID, std::filesystem::path> m_UUIDToPath;
    tsl::robin_map<std::string, AssetUUID> m_PathToUUID;
    tsl::robin_map<AssetUUID, AssetMeta> m_Metas;
};

}