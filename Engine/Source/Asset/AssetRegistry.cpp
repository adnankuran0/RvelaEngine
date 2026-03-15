#include "rvelapch.h"
#include "AssetRegistry.h"
#include "Core/Log.h"
#include <fstream>

using namespace rv;

void AssetRegistry::Scan(const std::filesystem::path& assetDir)
{
    m_AssetDir = assetDir;

    for (auto& entry : std::filesystem::recursive_directory_iterator(assetDir))
    {
        if (!entry.is_regular_file()) continue;

        auto path = entry.path();
        auto ext = path.extension().string();

        if (ext == ".rmeta") continue;

        auto metaPath = AssetMeta::GetMetaPath(path);
        if (!std::filesystem::exists(metaPath)) continue;

        AssetMeta meta;
        if (!meta.LoadFromFile(metaPath)) continue;
        if (!meta.uuid.IsValid())         continue;

        m_UUIDToPath[meta.uuid] = path;
        m_PathToUUID[path.string()] = meta.uuid;
        m_Metas[meta.uuid] = std::move(meta);
    }

    LOG_INFO("[AssetRegistry] Scan complete. {} assets registered.", m_UUIDToPath.size());
}

AssetMeta AssetRegistry::GetMeta(const AssetUUID& uuid) const
{
    auto it = m_Metas.find(uuid);
    if (it == m_Metas.end())
    {
        LOG_WARN("Meta not found for UUID: {}", uuid.ToString());
        return {};
    }
    return it->second;
}

AssetMeta AssetRegistry::GetOrCreateMeta(const std::filesystem::path& path)
{
    auto pathIt = m_PathToUUID.find(path.string());
    if (pathIt != m_PathToUUID.end())
    {
        auto metaIt = m_Metas.find(pathIt->second);
        if (metaIt != m_Metas.end())
            return metaIt->second;
    }

    auto metaPath = AssetMeta::GetMetaPath(path);
    if (std::filesystem::exists(metaPath))
    {
        AssetMeta meta;
        if (meta.LoadFromFile(metaPath))
        {
            m_UUIDToPath[meta.uuid] = path;
            m_PathToUUID[path.string()] = meta.uuid;
            m_Metas[meta.uuid] = meta;
            return meta;
        }
    }

    // if meta doesnt exists
    AssetMeta meta;
    meta.uuid = AssetUUID{};
    meta.importerID = ""; // AssetImportPipeline
    meta.sourceHash = 0;

    meta.SaveToFile(metaPath);

    m_UUIDToPath[meta.uuid] = path;
    m_PathToUUID[path.string()] = meta.uuid;
    m_Metas[meta.uuid] = meta;

    return meta;
}

void AssetRegistry::SaveMeta(const std::filesystem::path& path, const AssetMeta& meta)
{
    auto metaPath = AssetMeta::GetMetaPath(path);
    meta.SaveToFile(metaPath);

    m_Metas[meta.uuid] = meta;
    m_UUIDToPath[meta.uuid] = path;
    m_PathToUUID[path.string()] = meta.uuid;
}

bool AssetRegistry::Exists(const AssetUUID& uuid) const
{
    return m_UUIDToPath.contains(uuid);
}

std::filesystem::path AssetRegistry::GetPath(const AssetUUID& uuid) const
{
    auto it = m_UUIDToPath.find(uuid);
    if (it == m_UUIDToPath.end())
    {
        LOG_WARN("Path not found for UUID: {}", uuid.ToString());
        return {};
    }
    return it->second;
}

AssetUUID AssetRegistry::GetUUID(const std::filesystem::path& path) const
{
    auto it = m_PathToUUID.find(path.string());
    if (it == m_PathToUUID.end())
        return AssetUUID::Invalid();
    return it->second;
}

std::vector<AssetUUID> AssetRegistry::GetDependencies(const AssetUUID& uuid) const
{
    auto it = m_Metas.find(uuid);
    if (it == m_Metas.end())
        return {};
    return it->second.dependencies;
}