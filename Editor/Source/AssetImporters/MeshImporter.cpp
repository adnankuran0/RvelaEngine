#include "rvelapch.h"
#include "MeshImporter.h"
#include "Asset/CacheTypes/MeshCacheHeader.h"
#include "Asset/AssetMeta.h"
#include "Asset/AssetRegistry.h"
#include "Renderer/AABB.h"
#include "Core/Log.h"
#include "Asset/AssetManager.h"
#include <fstream>
#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>
#include "meshoptimizer/meshoptimizer.h"

using namespace rv;

static std::string SanitizeFilename(const std::string& name)
{
    std::string result = name;
    for (char& c : result)
        if (c == ':' || c == '/' || c == '\\' || c == '?' ||
            c == '*' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    return result;
}

std::filesystem::path MeshImporter::GetCachePath(
    const std::filesystem::path& sourcePath,
    const AssetMeta& meta,
    const std::filesystem::path& cacheRoot) const
{
    return cacheRoot / (meta.uuid.ToString() + ".rmesh");
}

bool MeshImporter::Import(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& outCachePath,
    const std::string& settingsJson)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(sourcePath.string(),
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenBoundingBoxes);

    if (!scene || scene->mNumMeshes == 0)
    {
        LOG_ERROR("Failed to load: {}", sourcePath.string());
        return false;
    }

    aiMesh* mesh = scene->mMeshes[0];
    auto vertices = ProcessVertices(mesh);
    auto indices = ProcessIndices(mesh);
    OptimizeMesh(vertices, indices);

    aiVector3D min = mesh->mAABB.mMin;
    aiVector3D max = mesh->mAABB.mMax;
    AABB aabb(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, max.y, max.z));

    std::vector<MeshLODHeader> lodHeaders;
    std::vector<unsigned int> allIndices;
    GenerateLODs(vertices, indices, lodHeaders, allIndices);

    return WriteMeshCache(outCachePath, mesh->mName.C_Str(), vertices, allIndices, lodHeaders, aabb);
}

bool MeshImporter::ImportFromScene(
    const aiScene* scene,
    const std::filesystem::path& sourcePath,
    AssetRegistry& registry,
    std::unordered_map<unsigned int, AssetUUID>& outMeshMap,
    const std::vector<std::string>& meshNames,
    bool optimize)
{
    if (scene->mNumMeshes == 0) return false;

    auto cacheRoot = registry.GetAssetDir() / ".cache";
    std::filesystem::create_directories(cacheRoot);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        if (!outMeshMap.empty())
        {
            if (outMeshMap.find(i) == outMeshMap.end())
                continue;
        }
        else
        {
            if (mesh->mNumBones > 0)
                continue;
        }

        std::string meshName;
        if (i < meshNames.size() && !meshNames[i].empty())
            meshName = meshNames[i];
        else
        {
            meshName = mesh->mName.C_Str();
            if (meshName.empty()) meshName = "Mesh_" + std::to_string(i);
        }

        AssetUUID meshUUID;
        auto existIt = outMeshMap.find(i);
        if (existIt != outMeshMap.end() && existIt->second.IsValid())
            meshUUID = existIt->second;
        else
        {
            meshUUID = AssetUUID{};
            outMeshMap[i] = meshUUID;
        }

        auto cachePath = cacheRoot / (meshUUID.ToString() + ".rmesh");

        auto vertices = ProcessVertices(mesh);
        auto indices = ProcessIndices(mesh);
        if (vertices.empty() || indices.empty())
        {
            LOG_WARN("Mesh '{}' (index {}) has no valid vertices or triangles, skipping.", meshName, i);
            outMeshMap.erase(i);
            continue;
        }
        if (optimize)
        {
            OptimizeMesh(vertices, indices);
        }
        if (vertices.empty() || indices.empty())
        {
            LOG_WARN("Mesh '{}' (index {}) has no valid vertices or triangles after optimization, skipping.", meshName, i);
            outMeshMap.erase(i);
            continue;
        }
        aiVector3D mn = mesh->mAABB.mMin, mx = mesh->mAABB.mMax;
        AABB aabb(glm::vec3(mn.x, mn.y, mn.z), glm::vec3(mx.x, mx.y, mx.z));

        std::vector<MeshLODHeader> lodHeaders;
        std::vector<unsigned int> allIndices;
        GenerateLODs(vertices, indices, lodHeaders, allIndices);

        if (!WriteMeshCache(cachePath, meshName, vertices, allIndices, lodHeaders, aabb))
        {
            LOG_ERROR("Failed to write cache for mesh {}", i);
            outMeshMap.erase(i);
            continue;
        }

        registry.RegisterSubAsset(meshUUID, cachePath, "MeshImporter");
        AssetManager::Get().Unload(meshUUID);
    }

    return true;
}

void MeshImporter::GenerateLODs(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& baseIndices,
    std::vector<MeshLODHeader>& outLODHeaders,
    std::vector<unsigned int>& outAllIndices)
{
    outLODHeaders.clear();
    outAllIndices.clear();

    if (vertices.empty() || baseIndices.empty())
        return;

    MeshLODHeader lod0;
    lod0.indexOffset = 0;
    lod0.indexCount = static_cast<uint32_t>(baseIndices.size());
    lod0.screenRatio = 0.50f;
    lod0.lodError = 0.0f;
    outLODHeaders.push_back(lod0);

    outAllIndices.insert(outAllIndices.end(), baseIndices.begin(), baseIndices.end());

    size_t baseTriangles = baseIndices.size() / 3;
    // Dont simplify very small  meshes
    if (baseTriangles < 64)
        return;

    struct LODTarget {
        float ratio;
        float targetError;
        float screenRatio;
    };

    const LODTarget targets[] = {
        { 0.50f, 0.015f, 0.25f }, // LOD 1
        { 0.25f, 0.035f, 0.12f }, // LOD 2
        { 0.125f, 0.060f, 0.05f } // LOD 3
    };

    const float* vertexPositions = reinterpret_cast<const float*>(&vertices[0].position);
    size_t vertexCount = vertices.size();
    size_t vertexStride = sizeof(Vertex);

    uint32_t currentOffset = static_cast<uint32_t>(baseIndices.size());
    size_t lastIndexCount = baseIndices.size();

    for (const auto& target : targets)
    {
        size_t targetIndexCount = static_cast<size_t>(baseIndices.size() * target.ratio);
        targetIndexCount = (targetIndexCount / 3) * 3;
        if (targetIndexCount < 12)
            break;

        std::vector<unsigned int> simplifiedIndices(baseIndices.size());
        float resultError = 0.0f;

        size_t simplifiedIndexCount = meshopt_simplify(
            simplifiedIndices.data(),
            baseIndices.data(),
            baseIndices.size(),
            vertexPositions,
            vertexCount,
            vertexStride,
            targetIndexCount,
            target.targetError,
            0,
            &resultError
        );

        if (simplifiedIndexCount == 0 || simplifiedIndexCount > lastIndexCount * 0.85f)
        {
            if (target.ratio <= 0.15f)
            {
                simplifiedIndexCount = meshopt_simplifySloppy(
                    simplifiedIndices.data(),
                    baseIndices.data(),
                    baseIndices.size(),
                    vertexPositions,
                    vertexCount,
                    vertexStride,
                    targetIndexCount,
                    target.targetError * 1.5f,
                    &resultError
                );
            }
            if (simplifiedIndexCount == 0 || simplifiedIndexCount > lastIndexCount * 0.85f)
                break;
        }

        simplifiedIndices.resize(simplifiedIndexCount);

        meshopt_optimizeVertexCache(
            simplifiedIndices.data(),
            simplifiedIndices.data(),
            simplifiedIndexCount,
            vertexCount
        );

        MeshLODHeader lodHeader;
        lodHeader.indexOffset = currentOffset;
        lodHeader.indexCount = static_cast<uint32_t>(simplifiedIndexCount);
        lodHeader.screenRatio = target.screenRatio;
        lodHeader.lodError = resultError;
        outLODHeaders.push_back(lodHeader);

        outAllIndices.insert(outAllIndices.end(), simplifiedIndices.begin(), simplifiedIndices.end());
        currentOffset += static_cast<uint32_t>(simplifiedIndexCount);
        lastIndexCount = simplifiedIndexCount;
    }
}

bool MeshImporter::WriteMeshCache(
    const std::filesystem::path& outPath,
    const std::string& meshName,
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& allIndices,
    const std::vector<MeshLODHeader>& lodHeaders,
    const AABB& aabb)
{
    std::filesystem::create_directories(outPath.parent_path());

    std::ofstream file(outPath, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Cannot write: {}", outPath.string());
        return false;
    }

    MeshCacheHeader header{};
    header.magic = MAGIC_MESH;
    header.version = MESH_CACHE_VERSION;
    header.vertexCount = static_cast<uint32_t>(vertices.size());
    header.indexCount = static_cast<uint32_t>(allIndices.size());
    header.lodCount = static_cast<uint16_t>(lodHeaders.size());

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(&aabb), sizeof(AABB));

    uint32_t nameLen = static_cast<uint32_t>(meshName.size());
    file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    file.write(meshName.data(), nameLen);

    if (!lodHeaders.empty())
    {
        file.write(reinterpret_cast<const char*>(lodHeaders.data()),
            lodHeaders.size() * sizeof(MeshLODHeader));
    }

    file.write(reinterpret_cast<const char*>(vertices.data()),
        vertices.size() * sizeof(Vertex));
    file.write(reinterpret_cast<const char*>(allIndices.data()),
        allIndices.size() * sizeof(unsigned int));

    return file.good();
}

void MeshImporter::OptimizeMesh(std::vector<Vertex>& outVertices, std::vector<unsigned int>& outIndices)
{
    size_t vertexCount = outVertices.size();
    size_t indexCount = outIndices.size();

    if (vertexCount == 0 || indexCount < 3 || (indexCount % 3) != 0) return;

    std::vector<unsigned int> remap(vertexCount);
    size_t newVertexCount = meshopt_generateVertexRemap(
        remap.data(),
        outIndices.data(),
        indexCount,
        outVertices.data(),
        vertexCount,
        sizeof(Vertex));

    if (newVertexCount == 0) return;

    std::vector<unsigned int> remappedIndices(indexCount);
    std::vector<Vertex> remappedVertices(newVertexCount);

    meshopt_remapIndexBuffer(
        remappedIndices.data(),
        outIndices.data(),
        indexCount,
        remap.data());

    meshopt_remapVertexBuffer(
        remappedVertices.data(),
        outVertices.data(),
        vertexCount,
        sizeof(Vertex),
        remap.data());

    outIndices = std::move(remappedIndices);
    outVertices = std::move(remappedVertices);
    vertexCount = newVertexCount;

    meshopt_optimizeVertexCache(
        outIndices.data(),
        outIndices.data(),
        indexCount,
        vertexCount);

    meshopt_optimizeOverdraw(
        outIndices.data(),
        outIndices.data(),
        indexCount,
        reinterpret_cast<const float*>(outVertices.data()),
        vertexCount,
        sizeof(Vertex),
        1.05f);

    size_t finalVertexCount = meshopt_optimizeVertexFetch(
        outVertices.data(),
        outIndices.data(),
        indexCount,
        outVertices.data(),
        vertexCount,
        sizeof(Vertex));

    outVertices.resize(finalVertexCount);
}

std::vector<Vertex> MeshImporter::ProcessVertices(aiMesh* mesh)
{
    std::vector<Vertex> vertices(mesh->mNumVertices);
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
        {
            vertices[i].tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        }

        vertices[i].texCoord = hasTexCoords
            ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
            : glm::vec2(0.0f);
    }

    return vertices;
}

std::vector<unsigned int> MeshImporter::ProcessIndices(aiMesh* mesh)
{
    std::vector<unsigned int> indices;
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        if (face.mNumIndices != 3)
            continue;

        if (face.mIndices[0] >= mesh->mNumVertices ||
            face.mIndices[1] >= mesh->mNumVertices ||
            face.mIndices[2] >= mesh->mNumVertices)
            continue;

        indices.insert(indices.end(), face.mIndices, face.mIndices + 3);
    }
    return indices;
}
