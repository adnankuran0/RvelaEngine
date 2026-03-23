#pragma once
#include "Asset/IAssetImporter.h"
#include <unordered_map>
#include "Asset/Types/MeshAsset.h"

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
        std::unordered_map<unsigned int, AssetUUID>& outMeshMap);

    std::string GetDefaultSettings() const override { return ""; }

private:
    std::vector<Vertex>       ProcessVertices(aiMesh* mesh);
    std::vector<unsigned int> ProcessIndices(aiMesh* mesh);

    bool WriteMeshCache(
        const std::filesystem::path& outPath,
        const std::string& meshName,
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        const AABB& aabb);
};

}