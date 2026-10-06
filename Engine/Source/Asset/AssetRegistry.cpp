#include "rvelapch.h"
#include "AssetRegistry.h"
#include "Asset/CacheTypes/TextureCacheHeader.h"
#include "Core/Log.h"
#include <fstream>
#include <algorithm>
#include <cctype>

using namespace rv;

void AssetRegistry::Clear()
{
    m_AssetDir.clear();
    m_UUIDToPath.clear();
    m_PathToUUID.clear();
    m_Metas.clear();
}

void AssetRegistry::Scan(const std::filesystem::path& assetDir)
{
    Clear();
    m_AssetDir = assetDir;

    std::error_code ec;
    if (assetDir.empty() || !std::filesystem::exists(assetDir, ec) || !std::filesystem::is_directory(assetDir, ec))
    {
        LOG_WARN("[AssetRegistry::Scan] Directory does not exist or is not a directory: {}", assetDir.string());
        return;
    }

    auto cacheRoot = assetDir / ".cache";

    static const std::unordered_set<std::string> s_TrackOnlyExtensions = {
        ".rscene", ".lua", ".mp3", ".wav"
    };

    for (auto& entry : std::filesystem::recursive_directory_iterator(assetDir, std::filesystem::directory_options::skip_permission_denied, ec))
    {
        if (ec)
        {
            LOG_ERROR("[AssetRegistry::Scan] Directory iteration error: {}", ec.message());
            break;
        }
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

            m_UUIDToPath[sub.uuid] = path;

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

            if (ext != ".rmesh" && ext != ".rskmesh" && ext != ".rtex" && ext != ".rprefab" && ext != ".rmat" )
                continue;

            auto uuid = AssetUUID::FromString(cachePath.stem().string());
            if (!uuid.IsValid())
            {
                LOG_WARN("Invalid UUID in filename");
                continue;
            }

            bool hasMeta = m_Metas.contains(uuid);
            bool hasPath = m_UUIDToPath.contains(uuid);

            if (ext == ".rtex" && hasPath)
            {
                auto sourcePath = m_UUIDToPath[uuid];
                std::string sourceExtension = sourcePath.extension().string();
                std::transform(sourceExtension.begin(), sourceExtension.end(), sourceExtension.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (sourceExtension == ".hdr")
                {
                    TextureCacheHeader header{};
                    std::ifstream cacheFile(cachePath, std::ios::binary);
                    cacheFile.read(reinterpret_cast<char*>(&header), sizeof(header));
                    const uint64_t channels = header.format == TextureFormat::RGB32F ? 3u
                        : header.format == TextureFormat::RGBA32F ? 4u : 0u;
                    const uint64_t expectedSize = static_cast<uint64_t>(header.width) * header.height * channels * sizeof(float);
                    const bool validHDRCache = cacheFile && header.magic == MAGIC_TEXTURE &&
                        header.version == TEXTURE_CACHE_VERSION && channels != 0 && !header.isSRGB &&
                        header.mipCount == 1 && header.dataSize == expectedSize;
                    if (!validHDRCache)
                    {
                        LOG_WARN("Ignoring outdated or invalid HDR texture cache: {}", cachePath.string());
                        continue;
                    }
                }
            }

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
            for (auto& sub : meta.subAssets)
            {
                if (sub.uuid.IsValid())
                    m_UUIDToPath[sub.uuid] = path;
            }
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

        m_UUIDToPath[sub.uuid] = path;

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
    std::string pathStr = path.string();

    auto it = m_PathToUUID.find(pathStr);
    if (it != m_PathToUUID.end())
        return it->second;

    std::string genericStr = path.generic_string();
    std::string relativeToAssets = genericStr;
    if (relativeToAssets.starts_with("Assets/"))
        relativeToAssets = relativeToAssets.substr(7);

    if (!m_AssetDir.empty())
    {
        auto combined = (m_AssetDir / relativeToAssets).lexically_normal();
        it = m_PathToUUID.find(combined.string());
        if (it != m_PathToUUID.end())
            return it->second;
    }

    for (const auto& [uuid, fullPath] : m_UUIDToPath)
    {
        std::string fullGeneric = fullPath.generic_string();
        if (fullGeneric.ends_with(genericStr) ||
            fullGeneric.ends_with(relativeToAssets) ||
            fullPath.filename().generic_string() == genericStr)
        {
            return uuid;
        }
    }

    if (pathStr.length() == 36 && pathStr[8] == '-' && pathStr[13] == '-' && pathStr[18] == '-' && pathStr[23] == '-')
    {
        AssetUUID parsed = AssetUUID::FromString(pathStr);
        if (parsed.IsValid())
            return parsed;
    }

    return AssetUUID::Invalid();
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
