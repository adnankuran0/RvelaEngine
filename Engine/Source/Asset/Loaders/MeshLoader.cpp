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

    if (header.version != MESH_CACHE_VERSION)
    {
        LOG_ERROR("MeshLoader::LoadFromMemory: Version mismatch (got {}, expected {})",
            header.version, MESH_CACHE_VERSION);
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
