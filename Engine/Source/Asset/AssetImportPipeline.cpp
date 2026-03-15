#include "rvelapch.h"
#include "AssetImportPipeline.h"
#include "AssetRegistry.h"
#include "AssetMeta.h"
#include "IAssetImporter.h"
#include "Core/Log.h"

using namespace rv;

void AssetImportPipeline::RegisterImporter(std::unique_ptr<IAssetImporter> importer)
{
    m_Importers.push_back(std::move(importer));
}

bool AssetImportPipeline::ImportAsset(const std::filesystem::path& sourcePath, AssetRegistry& registry)
{
    auto ext = sourcePath.extension().string();

    IAssetImporter* importer = FindImporter(ext);
    if (!importer)
    {
        LOG_WARN("No importer for extension: {}", ext);
        return false;
    }

    AssetMeta meta = registry.GetOrCreateMeta(sourcePath);

    // set importer id for first laded assets
    if (meta.importerID.empty())
    {
        meta.importerID = importer->GetImporterID();
        registry.SaveMeta(sourcePath, meta);
    }

    if (!NeedsReimport(sourcePath, meta))
    {
        return true;
    }

    auto cachePath = importer->GetCachePath(sourcePath, meta);

    std::filesystem::create_directories(cachePath.parent_path());

    bool ok = importer->Import(sourcePath, cachePath, meta.importerSettingsJson);
    if (!ok)
    {
        LOG_ERROR("Import failed: {}", sourcePath.string());
        return false;
    }

    meta.sourceHash = IAssetImporter::HashFile(sourcePath);
    registry.SaveMeta(sourcePath, meta);

    return true;
}

void AssetImportPipeline::ReimportAll(AssetRegistry& registry)
{
    int success = 0;
    int failed = 0;

    for (auto& entry : std::filesystem::recursive_directory_iterator(registry.GetAssetDir()))
    {
        if (!entry.is_regular_file()) continue;

        auto path = entry.path();
        auto ext = path.extension().string();

        if (ext == ".rmeta") continue;

        IAssetImporter* importer = FindImporter(ext);
        if (!importer) continue;

        if (ImportAsset(path, registry))
            ++success;
        else
            ++failed;
    }
}

bool AssetImportPipeline::NeedsReimport(const std::filesystem::path& sourcePath, const AssetMeta& meta) const
{
    if (meta.sourceHash == 0)
        return true;

    IAssetImporter* importer = FindImporter(sourcePath.extension().string());
    if (!importer)
        return false;

    auto cachePath = importer->GetCachePath(sourcePath, meta);
    if (!std::filesystem::exists(cachePath))
        return true;

    uint64_t currentHash = IAssetImporter::HashFile(sourcePath);
    return currentHash != meta.sourceHash;
}

IAssetImporter* AssetImportPipeline::FindImporter(const std::string& ext) const
{
    for (auto& importer : m_Importers)
        for (auto& supported : importer->GetSupportedExtensions())
            if (supported == ext)
                return importer.get();
    return nullptr;
}