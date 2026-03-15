#pragma once
#include <memory>
#include "IAssetImporter.h"

namespace rv {

class AssetRegistry;
class AssetMeta;

class AssetImportPipeline
{
public:
    void RegisterImporter(std::unique_ptr<IAssetImporter> importer);

    bool ImportAsset(const std::filesystem::path& sourcePath, AssetRegistry& db);
    void ReimportAll(AssetRegistry& db);
    bool NeedsReimport(const std::filesystem::path& sourcePath, const AssetMeta& meta) const;

private:
    IAssetImporter* FindImporter(const std::string& ext) const;
    std::vector<std::unique_ptr<IAssetImporter>> m_Importers;
};

}