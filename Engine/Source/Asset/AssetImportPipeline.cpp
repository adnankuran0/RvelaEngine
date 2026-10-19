#include "rvelapch.h"
#include "AssetImportPipeline.h"
#include "AssetRegistry.h"
#include "AssetMeta.h"
#include "IAssetImporter.h"
#include "Core/Log.h"
#include "Asset/AssetManager.h"
#include "Renderer/TextureCache.h"
#include <fstream>
#include <chrono>

using namespace rv;

void AssetImportPipeline::RegisterImporter(std::unique_ptr<IAssetImporter> importer)
{
    m_Importers.push_back(std::move(importer));
}

bool AssetImportPipeline::ImportAsset(const std::filesystem::path& sourcePath, AssetRegistry& registry, bool force)
{
    auto ext = sourcePath.extension().string();

    if (ext == ".bin")
        return false;

    IAssetImporter* importer = FindImporter(ext);
    if (!importer)
    {
        LOG_WARN("No importer for extension: {}", ext);
        return false;
    }

    AssetMeta meta = registry.GetOrCreateMeta(sourcePath);

    if (meta.importerID.empty())
    {
        meta.importerID = importer->GetImporterID();
        meta.importerSettingsJson = importer->GetDefaultSettings();
        registry.SaveMeta(sourcePath, meta);
    }

    if (!force && !NeedsReimport(sourcePath, meta))
    {
        return true;
    }

    auto cacheRoot = registry.GetAssetDir() / ".cache";
    std::filesystem::create_directories(cacheRoot);
    auto cachePath = importer->GetCachePath(sourcePath, meta, cacheRoot);

    std::filesystem::create_directories(cachePath.parent_path());

    bool ok = importer->Import(sourcePath, cachePath, meta.importerSettingsJson);
    if (!ok)
    {
        LOG_ERROR("Import failed: {}", sourcePath.string());
        return false;
    }

    std::error_code ec;
    auto lastWrite = std::filesystem::last_write_time(sourcePath, ec);
    if (!ec)
    {
        meta.lastWriteTime = std::chrono::duration_cast<std::chrono::seconds>(
            lastWrite.time_since_epoch()).count();
    }
    else
    {
        meta.lastWriteTime = 0;
    }

    auto updatedMeta = registry.GetMeta(meta.uuid);
    updatedMeta.lastWriteTime = meta.lastWriteTime;
    updatedMeta.importerVersion = importer->GetImporterVersion(sourcePath);
    registry.SaveMeta(sourcePath, updatedMeta);
    registry.RegisterPath(updatedMeta.uuid, cachePath);

    TextureCache::Get().Remove(updatedMeta.uuid);

    AssetManager::Get().Reload(updatedMeta.uuid);

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

        if (ImportAsset(path, registry, /*force=*/true))
            ++success;
        else
            ++failed;
    }
}

void AssetImportPipeline::Reimport(const std::filesystem::path& sourcePath, AssetRegistry& registry)
{
    auto ext = sourcePath.extension().string();
    if (ext == ".rmeta") return;

    IAssetImporter* importer = FindImporter(sourcePath.extension().string());
    if (!importer) return;

    ImportAsset(sourcePath, registry, /*force=*/true);
}

bool AssetImportPipeline::CheckAndReimportModifiedAssets(AssetRegistry& registry)
{
    static auto s_LastCheckTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_LastCheckTime).count() < 1000)
        return false;
    s_LastCheckTime = now;

    if (registry.GetAssetDir().empty())
        return false;

    std::error_code ec;
    if (!std::filesystem::exists(registry.GetAssetDir(), ec))
        return false;

    bool anyReimported = false;
    for (auto& entry : std::filesystem::recursive_directory_iterator(
        registry.GetAssetDir(),
        std::filesystem::directory_options::skip_permission_denied, ec))
    {
        if (ec) break;
        if (!entry.is_regular_file()) continue;

        auto path = entry.path().lexically_normal();
        auto ext = path.extension().string();
        if (ext == ".rmeta" || ext.empty()) continue;
        if (path.string().find(".cache") != std::string::npos) continue;

        IAssetImporter* importer = FindImporter(ext);
        if (!importer) continue;

        static std::unordered_map<std::string, std::chrono::steady_clock::time_point> s_LastReimportAttempts;
        auto attemptIt = s_LastReimportAttempts.find(path.string());
        if (attemptIt != s_LastReimportAttempts.end())
        {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - attemptIt->second).count() < 5)
                continue;
        }

        AssetMeta meta = registry.GetOrCreateMeta(path);
        if (NeedsReimport(path, meta))
        {
            // Verify file is not currently locked by an external exporter
            {
                std::ifstream test(path, std::ios::binary);
                if (!test.is_open())
                    continue;
            }

            s_LastReimportAttempts[path.string()] = now;
            LOG_INFO("[AssetImportPipeline] Auto-reimporting modified asset: {}", path.filename().string());
            if (ImportAsset(path, registry, /*force=*/true))
            {
                anyReimported = true;
            }
        }
    }

    return anyReimported;
}

bool AssetImportPipeline::NeedsReimport(const std::filesystem::path& sourcePath, const AssetMeta& meta) const
{
    if (meta.lastWriteTime == 0)
        return true;

    if (meta.IsModified(sourcePath))
        return true;

    IAssetImporter* importer = FindImporter(sourcePath.extension().string());
    if (!importer)
        return false;

    if (meta.importerVersion != importer->GetImporterVersion(sourcePath))
        return true;

    auto& registry = AssetManager::Get().GetRegistry();
    auto cacheRoot = registry.GetAssetDir() / ".cache";
    auto cachePath = importer->GetCachePath(sourcePath, meta, cacheRoot);

    if (!std::filesystem::exists(cachePath))
        return true;

    for (auto& sub : meta.subAssets)
    {
        if (!sub.uuid.IsValid()) continue;
        if (!sub.hasCache) continue;

        auto subCachePath = registry.GetPath(sub.uuid);
        if (subCachePath.empty())
            return true;

        if (!std::filesystem::exists(subCachePath))
            return true;
    }

    return false;
}

IAssetImporter* AssetImportPipeline::FindImporter(const std::string& ext) const
{
    for (auto& importer : m_Importers)
        for (auto& supported : importer->GetSupportedExtensions())
            if (supported == ext)
                return importer.get();
    return nullptr;
}
