#pragma once
#include <memory>
#include "IAssetImporter.h"

namespace rv {

class AssetRegistry;
struct AssetMeta;

class AssetImportPipeline
{
public:
    void RegisterImporter(std::unique_ptr<IAssetImporter> importer);

    bool ImportAsset(const std::filesystem::path& sourcePath, AssetRegistry& registry, bool force = false);
    void ReimportAll(AssetRegistry& db);
    void Reimport(const std::filesystem::path& sourcePath, AssetRegistry& registry);
    bool NeedsReimport(const std::filesystem::path& sourcePath, const AssetMeta& meta) const;
    bool CheckAndReimportModifiedAssets(AssetRegistry& registry);

private:
    IAssetImporter* FindImporter(const std::string& ext) const;
    std::vector<std::unique_ptr<IAssetImporter>> m_Importers;
};

}