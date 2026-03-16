#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "AssetUUID.h"

namespace rv {

struct SubAssetEntry 
{
    AssetUUID uuid;
    std::string name;
    std::string type;
    uint32_t index = 0;
    uint64_t sourceHash = 0;
};

struct AssetMeta
{
    AssetUUID uuid;
    std::string importerID;
    uint64_t sourceHash = 0;
    std::string importerSettingsJson;

    std::vector<AssetUUID> dependencies;  
    std::vector<SubAssetEntry> subAssets;    

    bool SaveToFile(const std::filesystem::path& metaPath) const;
    bool LoadFromFile(const std::filesystem::path& metaPath);

    static std::filesystem::path GetMetaPath(const std::filesystem::path& assetPath)
    {
        return std::filesystem::path(assetPath.string() + ".rmeta");
    }
};

}