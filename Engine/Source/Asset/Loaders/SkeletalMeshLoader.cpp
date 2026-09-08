#include "rvelapch.h"
#include "SkeletalMeshLoader.h"
#include "Asset/Types/SkeletalMeshAsset.h"
#include "Asset/CacheTypes/SkeletalMeshCacheHeader.h"
#include "Core/Log.h"
#include <fstream>
#include "Asset/AssetMeta.h"

using namespace rv;

Ref<Asset> SkeletalMeshLoader::Load(
    const std::filesystem::path& assetPath,
    const AssetMeta& meta)
{
    std::ifstream file(assetPath, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Cannot open: {}", assetPath.string());
        return nullptr;
    }

    SkeletalMeshCacheHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file)
    {
        LOG_ERROR("Failed to read header: {}", assetPath.string());
        return nullptr;
    }
    if (header.magic != MAGIC_SKMESH)
    {
        LOG_ERROR("Invalid magic: {}", assetPath.string());
        return nullptr;
    }
    if (header.version != SKMESH_CACHE_VERSION)
    {
        LOG_ERROR("Version mismatch (got {}, expected {}): {}",
            header.version, SKMESH_CACHE_VERSION, assetPath.string());
        return nullptr;
    }
    if (header.vertexCount == 0 || header.indexCount == 0)
    {
        LOG_ERROR("Empty skeletal mesh: {}", assetPath.string());
        return nullptr;
    }

    AABB aabb;
    file.read(reinterpret_cast<char*>(&aabb), sizeof(AABB));

    uint32_t nameLen = 0;
    file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    std::string meshName(nameLen, '\0');
    file.read(meshName.data(), nameLen);

    std::vector<SkeletalVertex> vertices(header.vertexCount);
    file.read(reinterpret_cast<char*>(vertices.data()),
        header.vertexCount * sizeof(SkeletalVertex));

    std::vector<unsigned int> indices(header.indexCount);
    file.read(reinterpret_cast<char*>(indices.data()),
        header.indexCount * sizeof(unsigned int));

    if (!file)
        return nullptr;

    auto asset = CreateRef<SkeletalMeshAsset>(meta.uuid);
    asset->m_LocalAABB = aabb;
    asset->m_MeshName = std::move(meshName);
    asset->m_Vertices = std::move(vertices);
    asset->m_Indices = std::move(indices);
    return asset;
}