#include "rvelapch.h"
#include "SkeletalMeshImporter.h"
#include "Asset/CacheTypes/SkeletalMeshCacheHeader.h"
#include "Asset/AssetMeta.h"
#include "Asset/AssetRegistry.h"
#include "Renderer/AABB.h"
#include "Core/Log.h"
#include <fstream>
#include <algorithm>
#include "meshoptimizer/meshoptimizer.h"

using namespace rv;

std::vector<unsigned int> SkeletalMeshImporter::ProcessIndices(aiMesh* mesh) const
{
    std::vector<unsigned int> indices;
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }
    return indices;
}

std::vector<SkeletalVertex> SkeletalMeshImporter::ProcessVertices(
    aiMesh* mesh,
    const std::unordered_map<std::string, int32_t>& boneNameToIndex) const
{
    std::vector<SkeletalVertex> vertices(mesh->mNumVertices);
    bool hasNormals = mesh->HasNormals();
    bool hasTangents = mesh->HasTangentsAndBitangents();
    bool hasTexCoords = mesh->mTextureCoords[0] != nullptr;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        const auto& pos = mesh->mVertices[i];
        vertices[i].position = glm::vec3(pos.x, pos.y, pos.z);

        vertices[i].normal = hasNormals
            ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
            : glm::vec3(0.0f);

        if (hasTangents)
            vertices[i].tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);

        vertices[i].texCoord = hasTexCoords
            ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
            : glm::vec2(0.0f);
    }

    std::vector<std::vector<BoneWeight>> influences(mesh->mNumVertices);

    for (uint32_t b = 0; b < mesh->mNumBones; ++b)
    {
        aiBone* bone = mesh->mBones[b];
        std::string name = bone->mName.C_Str();

        auto it = boneNameToIndex.find(name);
        if (it == boneNameToIndex.end())
        {
            LOG_WARN("Skeletal mesh '{}': bone '{}' was not found in the skeleton, skipping", mesh->mName.C_Str(), name);
            continue;
        }
        int32_t boneIndex = it->second;

        for (uint32_t w = 0; w < bone->mNumWeights; ++w)
        {
            const auto& vw = bone->mWeights[w];
            if (vw.mVertexId >= mesh->mNumVertices) continue;
            influences[vw.mVertexId].push_back({ boneIndex, vw.mWeight });
        }
    }

    uint32_t unweightedCount = 0;
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        auto& infl = influences[i];

        if (infl.empty())
        {
            vertices[i].boneIDs[0] = 0;
            vertices[i].weights[0] = 1.0f;
            ++unweightedCount;
            continue;
        }

        std::sort(infl.begin(), infl.end(),
            [](const BoneWeight& a, const BoneWeight& b) { return a.weight > b.weight; });

        size_t count = std::min<size_t>(infl.size(), 4);
        float sum = 0.0f;
        for (size_t k = 0; k < count; ++k)
            sum += infl[k].weight;

        if (sum <= 0.0f)
        {
            vertices[i].boneIDs[0] = 0;
            vertices[i].weights[0] = 1.0f;
            continue;
        }

        for (size_t k = 0; k < count; ++k)
        {
            vertices[i].boneIDs[k] = static_cast<uint32_t>(infl[k].boneIndex);
            vertices[i].weights[k] = infl[k].weight / sum;
        }
    }

    if (unweightedCount > 0)
        LOG_WARN("Skeletal mesh '{}': {} vertices have no bone weights and were assigned to the root bone", mesh->mName.C_Str(), unweightedCount);

    return vertices;
}

void SkeletalMeshImporter::OptimizeMesh(
    std::vector<SkeletalVertex>& outVertices,
    std::vector<unsigned int>& outIndices) const
{
    size_t vertexCount = outVertices.size();
    size_t indexCount = outIndices.size();
    if (vertexCount == 0 || indexCount == 0) return;

    std::vector<unsigned int> remap(vertexCount);
    size_t newVertexCount = meshopt_generateVertexRemap(
        remap.data(), outIndices.data(), indexCount,
        outVertices.data(), vertexCount, sizeof(SkeletalVertex));

    std::vector<unsigned int> remappedIndices(indexCount);
    std::vector<SkeletalVertex> remappedVertices(newVertexCount);

    meshopt_remapIndexBuffer(remappedIndices.data(), outIndices.data(), indexCount, remap.data());
    meshopt_remapVertexBuffer(remappedVertices.data(), outVertices.data(), vertexCount,
        sizeof(SkeletalVertex), remap.data());

    outIndices = std::move(remappedIndices);
    outVertices = std::move(remappedVertices);
    vertexCount = newVertexCount;

    meshopt_optimizeVertexCache(outIndices.data(), outIndices.data(), indexCount, vertexCount);

    meshopt_optimizeOverdraw(
        outIndices.data(), outIndices.data(), indexCount,
        &outVertices[0].position.x, vertexCount, sizeof(SkeletalVertex), 1.05f);

    meshopt_optimizeVertexFetch(
        outVertices.data(), outIndices.data(), indexCount,
        outVertices.data(), vertexCount, sizeof(SkeletalVertex));
}

bool SkeletalMeshImporter::WriteMeshCache(
    const std::filesystem::path& outPath,
    const std::string& meshName,
    const std::vector<SkeletalVertex>& vertices,
    const std::vector<unsigned int>& indices,
    const AABB& aabb) const
{
    std::filesystem::create_directories(outPath.parent_path());

    std::ofstream file(outPath, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Cannot write: {}", outPath.string());
        return false;
    }

    SkeletalMeshCacheHeader header{};
    header.vertexCount = static_cast<uint32_t>(vertices.size());
    header.indexCount = static_cast<uint32_t>(indices.size());

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(&aabb), sizeof(AABB));

    uint32_t nameLen = static_cast<uint32_t>(meshName.size());
    file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    file.write(meshName.data(), nameLen);

    file.write(reinterpret_cast<const char*>(vertices.data()),
        vertices.size() * sizeof(SkeletalVertex));
    file.write(reinterpret_cast<const char*>(indices.data()),
        indices.size() * sizeof(unsigned int));

    return file.good();
}

bool SkeletalMeshImporter::ImportFromScene(
    const aiScene* scene,
    const std::filesystem::path& sourcePath,
    AssetRegistry& registry,
    const std::unordered_map<std::string, int32_t>& boneNameToIndex,
    std::unordered_map<unsigned int, AssetUUID>& outSkeletalMeshMap)
{
    if (scene->mNumMeshes == 0 || boneNameToIndex.empty()) return false;

    auto cacheRoot = registry.GetAssetDir() / ".cache";
    std::filesystem::create_directories(cacheRoot);

    AssetMeta parentMeta = registry.GetOrCreateMeta(sourcePath);

    std::unordered_map<uint32_t, SubAssetEntry> existingByIndex;
    for (auto& sub : parentMeta.subAssets)
        if (sub.type == "SkeletalMesh")
            existingByIndex[sub.index] = sub;

    bool any = false;
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        if (mesh->mNumBones == 0) continue;

        std::string meshName = mesh->mName.C_Str();
        if (meshName.empty()) meshName = "SkeletalMesh_" + std::to_string(i);

        AssetUUID meshUUID;
        auto existIt = existingByIndex.find(i);
        if (existIt != existingByIndex.end())
            meshUUID = existIt->second.uuid;
        else
            meshUUID = AssetUUID{};

        auto cachePath = cacheRoot / (meshUUID.ToString() + ".rskmesh");
        bool needsImport = !std::filesystem::exists(cachePath);

        if (needsImport)
        {
            auto vertices = ProcessVertices(mesh, boneNameToIndex);
            auto indices = ProcessIndices(mesh);
            OptimizeMesh(vertices, indices);

            aiVector3D mn = mesh->mAABB.mMin, mx = mesh->mAABB.mMax;
            AABB aabb(glm::vec3(mn.x, mn.y, mn.z), glm::vec3(mx.x, mx.y, mx.z));

            if (!WriteMeshCache(cachePath, meshName, vertices, indices, aabb))
            {
                LOG_ERROR("Failed to write skeletal mesh cache for mesh {}", i);
                continue;
            }
        }

        registry.RegisterSubAsset(meshUUID, cachePath, "SkeletalMeshImporter");
        outSkeletalMeshMap[i] = meshUUID;
        any = true;
    }

    return any;
}