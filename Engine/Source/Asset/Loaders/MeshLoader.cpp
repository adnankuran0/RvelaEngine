#include "rvelapch.h"
#include "MeshLoader.h"
#include "Asset/Types/MeshAsset.h"
#include "Asset/CacheTypes/MeshCacheHeader.h"
#include "Renderer/AABB.h"
#include "Core/Log.h"
#include "Asset/AssetMeta.h"
#include <fstream>
#include <cstring>

namespace rv {

Ref<Asset> MeshLoader::LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta)
{
    if (!data || size < sizeof(MeshCacheHeader))
    {
        LOG_ERROR("MeshLoader::LoadFromMemory: Buffer too small");
        return nullptr;
    }

    size_t offset = 0;
    MeshCacheHeader header{};
    std::memcpy(&header, data + offset, sizeof(header));
    offset += sizeof(header);

    if (header.magic != MAGIC_MESH)
    {
        LOG_ERROR("MeshLoader::LoadFromMemory: Invalid magic (got 0x{:X})", header.magic);
        return nullptr;
    }

    if (header.version != MESH_CACHE_VERSION && header.version != MESH_CACHE_VERSION_1)
    {
        LOG_ERROR("MeshLoader::LoadFromMemory: Version mismatch (got {}, expected {} or {})",
            header.version, MESH_CACHE_VERSION, MESH_CACHE_VERSION_1);
        return nullptr;
    }

    if (header.vertexCount == 0 || header.indexCount == 0)
    {
        LOG_ERROR("MeshLoader::LoadFromMemory: Empty mesh");
        return nullptr;
    }

    if (offset + sizeof(AABB) + sizeof(uint32_t) > size)
    {
        LOG_ERROR("MeshLoader::LoadFromMemory: Incomplete AABB/Name length");
        return nullptr;
    }

    AABB aabb;
    std::memcpy(&aabb, data + offset, sizeof(AABB));
    offset += sizeof(AABB);

    uint32_t nameLen = 0;
    std::memcpy(&nameLen, data + offset, sizeof(nameLen));
    offset += sizeof(nameLen);

    if (offset + nameLen > size)
    {
        LOG_ERROR("MeshLoader::LoadFromMemory: Incomplete mesh name");
        return nullptr;
    }

    std::string meshName(nameLen, '\0');
    if (nameLen > 0)
    {
        std::memcpy(meshName.data(), data + offset, nameLen);
        offset += nameLen;
    }

    std::vector<MeshLODInfo> lods;
    if (header.version >= MESH_CACHE_VERSION)
    {
        uint16_t lodCount = header.lodCount > 0 ? header.lodCount : 1;
        const size_t lodHeadersBytes = lodCount * sizeof(MeshLODHeader);
        if (offset + lodHeadersBytes > size)
        {
            LOG_ERROR("MeshLoader::LoadFromMemory: Incomplete LOD headers");
            return nullptr;
        }

        std::vector<MeshLODHeader> lodHeaders(lodCount);
        std::memcpy(lodHeaders.data(), data + offset, lodHeadersBytes);
        offset += lodHeadersBytes;

        lods.resize(lodCount);
        for (size_t i = 0; i < lodCount; ++i)
        {
            lods[i].indexOffset = lodHeaders[i].indexOffset;
            lods[i].indexCount = lodHeaders[i].indexCount;
            lods[i].screenRatio = lodHeaders[i].screenRatio;
            lods[i].lodError = lodHeaders[i].lodError;
        }
    }
    else
    {
        MeshLODInfo lod0;
        lod0.indexOffset = 0;
        lod0.indexCount = header.indexCount;
        lod0.screenRatio = 0.5f;
        lod0.lodError = 0.0f;
        lods.push_back(lod0);
    }

    const size_t verticesBytes = header.vertexCount * sizeof(Vertex);
    const size_t indicesBytes = header.indexCount * sizeof(unsigned int);

    if (offset + verticesBytes + indicesBytes > size)
    {
        LOG_ERROR("MeshLoader::LoadFromMemory: Incomplete vertices or indices");
        return nullptr;
    }

    std::vector<Vertex> vertices(header.vertexCount);
    std::memcpy(vertices.data(), data + offset, verticesBytes);
    offset += verticesBytes;

    std::vector<unsigned int> indices(header.indexCount);
    std::memcpy(indices.data(), data + offset, indicesBytes);
    offset += indicesBytes;

    auto asset = CreateRef<MeshAsset>(meta.uuid);
    asset->m_LocalAABB = aabb;
    asset->m_MeshName = std::move(meshName);
    asset->m_Vertices = std::move(vertices);
    asset->m_Indices = std::move(indices);
    asset->m_LODs = std::move(lods);

    return asset;
}

Ref<Asset> MeshLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        LOG_ERROR("Cannot open: {}", assetPath.string());
        return nullptr;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize < static_cast<std::streamsize>(sizeof(MeshCacheHeader)))
    {
        LOG_ERROR("File too small: {}", assetPath.string());
        return nullptr;
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
    {
        LOG_ERROR("Failed to read file: {}", assetPath.string());
        return nullptr;
    }

    return LoadFromMemory(buffer.data(), buffer.size(), meta);
}

}
