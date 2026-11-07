#pragma once
#include "Asset/IAssetImporter.h"
#include <unordered_map>
#include "Asset/Types/MeshAsset.h"
#include "Asset/CacheTypes/MeshCacheHeader.h"

struct aiScene;
struct aiMesh;

namespace rv {

class AssetRegistry;

class MeshImporter : public IAssetImporter
{
public:
    std::string GetImporterID() const override { return "MeshImporter"; }

    std::vector<std::string> GetSupportedExtensions() const override
    {
        return { ".fbx", ".obj", ".gltf", ".glb" };
    }

    virtual std::filesystem::path GetCachePath(
        const std::filesystem::path& sourcePath,
        const AssetMeta& meta,
        const std::filesystem::path& cacheRoot) const override;

    bool Import(
        const std::filesystem::path& sourcePath,
        const std::filesystem::path& outCachePath,
        const std::string& settingsJson) override;

    bool ImportFromScene(
        const aiScene* scene,
        const std::filesystem::path& sourcePath,
        AssetRegistry& registry,
        std::unordered_map<unsigned int, AssetUUID>& outMeshMap,
        const std::vector<std::string>& meshNames = {},
        bool optimize = true);

    std::string GetDefaultSettings() const override { return ""; }

private:
    std::vector<Vertex>       ProcessVertices(aiMesh* mesh);
    std::vector<unsigned int> ProcessIndices(aiMesh* mesh);

    void GenerateLODs(
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& baseIndices,
        std::vector<MeshLODHeader>& outLODHeaders,
        std::vector<unsigned int>& outAllIndices);

    bool WriteMeshCache(
        const std::filesystem::path& outPath,
        const std::string& meshName,
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& allIndices,
        const std::vector<MeshLODHeader>& lodHeaders,
        const AABB& aabb);

    void OptimizeMesh(std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices);
};

}
