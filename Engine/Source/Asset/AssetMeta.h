#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "AssetUUID.h"

namespace rv {

struct ImporterSettings
{
    virtual ~ImporterSettings() = default;
    virtual std::string GetImporterID() const = 0;
};

struct AssetMeta
{
    AssetUUID   uuid;
    std::string importerID;     
    uint64_t    sourceHash = 0;  

    std::vector<AssetUUID> dependencies;

    std::string importerSettingsJson;

    bool SaveToFile(const std::filesystem::path& metaPath) const;
    bool LoadFromFile(const std::filesystem::path& metaPath);

    static std::filesystem::path GetMetaPath(const std::filesystem::path& assetPath)
    {
        return std::filesystem::path(assetPath.string() + ".rmeta");
    }
};

}