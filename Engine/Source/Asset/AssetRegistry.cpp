#include "rvelapch.h"
#include "AssetRegistry.h"
#include "Core/Log.h"
#include <fstream>

using namespace rv;

void AssetRegistry::Scan(const std::filesystem::path& assetDir)
{
    m_AssetDir = assetDir;
    auto cacheRoot = assetDir / ".cache";

    static const std::unordered_set<std::string> s_TrackOnlyExtensions = {
        ".rscene", ".lua"
    };

    for (auto& entry : std::filesystem::recursive_directory_iterator(assetDir))
    {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path();

        if (path.extension() == ".rmeta") continue;
        if (path.string().find((assetDir / ".cache").string()) != std::string::npos)
            continue;

        auto metaPath = AssetMeta::GetMetaPath(path);

        if (!std::filesystem::exists(metaPath))
        {
            if (s_TrackOnlyExtensions.count(path.extension().string()))
            {
                AssetMeta meta;
                meta.uuid = AssetUUID{};
                meta.importerID = "";
                meta.lastWriteTime = 0;
                meta.SaveToFile(metaPath);

                m_UUIDToPath[meta.uuid] = path;
                m_PathToUUID[path.string()] = meta.uuid;
                m_Metas[meta.uuid] = meta;
            }
            continue;
        }

        AssetMeta meta;
        if (!meta.LoadFromFile(metaPath))
        {
            LOG_WARN("Meta load failed: {}", metaPath.filename().string());
            continue;
        }
        if (!meta.uuid.IsValid())
        {
            LOG_WARN("Invalid UUID: {}", metaPath.filename().string());
            continue;
        }

       
        if (meta.lastWriteTime == 0)
        {
            std::error_code ec;
            auto lastWrite = std::filesystem::last_write_time(path, ec);
            if (!ec)
            {
                meta.lastWriteTime = std::chrono::duration_cast<std::chrono::seconds>(
                    lastWrite.time_since_epoch()).count();
                meta.SaveToFile(metaPath);
            }
        }

        m_UUIDToPath[meta.uuid] = path;
        m_PathToUUID[path.string()] = meta.uuid;
        m_Metas[meta.uuid] = meta;

        for (auto& sub : meta.subAssets)
        {
            if (!sub.uuid.IsValid()) continue;

            if (!m_Metas.contains(sub.uuid))
            {
                AssetMeta subMeta;
                subMeta.uuid = sub.uuid;
                subMeta.importerID = sub.type == "Mesh" ? "MeshImporter"
                    : sub.type == "Material" ? "MaterialLoader"
                    : "";
                m_Metas[sub.uuid] = subMeta;
            }
        }
    }

    if (std::filesystem::exists(cacheRoot))
    {
        for (auto& entry : std::filesystem::directory_iterator(cacheRoot))
        {
            if (!entry.is_regular_file()) continue;
            auto cachePath = entry.path();
            auto ext = cachePath.extension().string();

            if (ext != ".rmesh" && ext != ".rtex" && ext != ".rprefab" && ext != ".rmat")
                continue;

            auto uuid = AssetUUID::FromString(cachePath.stem().string());
            if (!uuid.IsValid())
            {
                LOG_WARN("Invalid UUID in filename");
                continue;
            }

            bool hasMeta = m_Metas.contains(uuid);
            bool hasPath = m_UUIDToPath.contains(uuid);

            if (!hasMeta)
            {
                std::error_code ec;
                std::filesystem::remove(cachePath, ec);

                if (ec)
                    LOG_ERROR("Failed to delete cache: {}", cachePath.string());

                continue;
            }

            bool alreadyHasCachePath = hasPath &&
                m_UUIDToPath[uuid].string().find(".cache") != std::string::npos;

            if (!alreadyHasCachePath)
            {
                m_UUIDToPath[uuid] = cachePath;
                m_PathToUUID[cachePath.string()] = uuid;
            }
        }
    }
    else
    {
        LOG_WARN(".cache does not exist: {}", cacheRoot.string());
    }
}

void AssetRegistry::RegisterSubAsset(const AssetUUID& uuid,
    const std::filesystem::path& cachePath,
    const std::string& importerID)
{
    m_UUIDToPath[uuid] = cachePath;
    m_PathToUUID[cachePath.string()] = uuid;

    auto it = m_Metas.find(uuid);
    if (it == m_Metas.end())
    {
        AssetMeta meta;
        meta.uuid = uuid;
        meta.importerID = importerID;
        m_Metas[uuid] = meta;
    }
    else
    {
        if (it.value().importerID.empty())
            it.value().importerID = importerID;
    }
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
            if (meta.lastWriteTime == 0)
            {
                std::error_code ec;
                auto lastWrite = std::filesystem::last_write_time(path, ec);
                if (!ec)
                {
                    meta.lastWriteTime = std::chrono::duration_cast<std::chrono::seconds>(
                        lastWrite.time_since_epoch()).count();
                    meta.SaveToFile(metaPath);
                }
            }

            m_UUIDToPath[meta.uuid] = path;
            m_PathToUUID[path.string()] = meta.uuid;
            m_Metas[meta.uuid] = meta;
            return meta;
        }
    }

    AssetMeta meta;
    meta.uuid = AssetUUID{}; // generates a new UUID
    meta.importerID = "";
    meta.lastWriteTime = 0;

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

    for (auto& sub : meta.subAssets)
    {
        if (!sub.uuid.IsValid()) continue;
        if (!m_Metas.contains(sub.uuid))
        {
            AssetMeta subMeta;
            subMeta.uuid = sub.uuid;
            subMeta.importerID = sub.type == "Mesh" ? "MeshImporter"
                : sub.type == "Material" ? "MaterialLoader"
                : "";
            m_Metas[sub.uuid] = subMeta;
        }
    }
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

void AssetRegistry::RegisterPath(const AssetUUID& uuid, const std::filesystem::path& path)
{
    m_UUIDToPath[uuid] = path;
    m_PathToUUID[path.string()] = uuid;
}
