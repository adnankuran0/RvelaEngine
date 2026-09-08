#pragma once
#include "Asset/AssetUUID.h"
#include "Asset/Types/SkeletalMeshAsset.h"
#include <Assimp/scene.h>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace rv {

class AssetRegistry;

class SkeletalMeshImporter
{
public:
    bool ImportFromScene(
        const aiScene* scene,
        const std::filesystem::path& sourcePath,
        AssetRegistry& registry,
        const std::unordered_map<std::string, int32_t>& boneNameToIndex,
        std::unordered_map<unsigned int, AssetUUID>& outSkeletalMeshMap);

private:
    struct BoneWeight { int32_t boneIndex; float weight; };

    std::vector<SkeletalVertex> ProcessVertices(
        aiMesh* mesh,
        const std::unordered_map<std::string, int32_t>& boneNameToIndex) const;

    std::vector<unsigned int> ProcessIndices(aiMesh* mesh) const;

    void OptimizeMesh(
        std::vector<SkeletalVertex>& outVertices,
        std::vector<unsigned int>& outIndices) const;

    bool WriteMeshCache(
        const std::filesystem::path& outPath,
        const std::string& meshName,
        const std::vector<SkeletalVertex>& vertices,
        const std::vector<unsigned int>& indices,
        const AABB& aabb) const;
};

}