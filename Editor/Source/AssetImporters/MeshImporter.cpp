#include "rvelapch.h"
#include "MeshImporter.h"
#include "Asset/CacheTypes/MeshCacheHeader.h"
#include "Asset/AssetMeta.h"
#include "Asset/AssetRegistry.h"
#include "Rendering/AABB.h"
#include "Core/Log.h"
#include <fstream>
#include <Assimp/Importer.hpp>
#include <Assimp/scene.h>
#include <Assimp/postprocess.h>

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

    aiVector3D min = mesh->mAABB.mMin;
    aiVector3D max = mesh->mAABB.mMax;
    AABB aabb(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, max.y, max.z));

    return WriteMeshCache(outCachePath, mesh->mName.C_Str(), vertices, indices, aabb);
}

bool MeshImporter::ImportFromScene(
    const aiScene* scene,
    const std::filesystem::path& sourcePath,
    AssetRegistry& registry,
    std::unordered_map<unsigned int, AssetUUID>& outMeshMap)
{
    if (scene->mNumMeshes == 0) return false;

    auto cacheRoot = registry.GetAssetDir() / ".cache";
    std::filesystem::create_directories(cacheRoot);

    AssetMeta parentMeta = registry.GetOrCreateMeta(sourcePath);
    std::unordered_map<uint32_t, SubAssetEntry> existingByIndex;
    for (auto& sub : parentMeta.subAssets)
        if (sub.type == "Mesh")
            existingByIndex[sub.index] = sub;

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];
        std::string meshName = mesh->mName.C_Str();
        if (meshName.empty()) meshName = "Mesh_" + std::to_string(i);

        uint64_t currentHash = IAssetImporter::HashFile(sourcePath)
            ^ (uint64_t(i) * 2654435761ULL);

        AssetUUID meshUUID;
        auto existIt = existingByIndex.find(i);
        if (existIt != existingByIndex.end())
            meshUUID = existIt->second.uuid;
        else
            meshUUID = AssetUUID{};

        auto cachePath = cacheRoot / (meshUUID.ToString() + ".rmesh");

        bool needsImport = !std::filesystem::exists(cachePath)
            || existIt == existingByIndex.end()
            || existIt->second.sourceHash != currentHash;

        if (needsImport)
        {
            auto vertices = ProcessVertices(mesh);
            auto indices = ProcessIndices(mesh);
            aiVector3D mn = mesh->mAABB.mMin, mx = mesh->mAABB.mMax;
            AABB aabb(glm::vec3(mn.x, mn.y, mn.z), glm::vec3(mx.x, mx.y, mx.z));

            if (!WriteMeshCache(cachePath, meshName, vertices, indices, aabb))
            {
                LOG_ERROR("Failed to write cache for mesh {}", i);
                continue;
            }
        }

        registry.RegisterSubAsset(meshUUID, cachePath, "MeshImporter");
        outMeshMap[i] = meshUUID;
    }

    return true;
}

bool MeshImporter::WriteMeshCache(
    const std::filesystem::path& outPath,
    const std::string& meshName,
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
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
    header.vertexCount = static_cast<uint32_t>(vertices.size());
    header.indexCount = static_cast<uint32_t>(indices.size());

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(&aabb), sizeof(AABB));

    uint32_t nameLen = static_cast<uint32_t>(meshName.size());
    file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    file.write(meshName.data(), nameLen);

    file.write(reinterpret_cast<const char*>(vertices.data()),
        vertices.size() * sizeof(Vertex));
    file.write(reinterpret_cast<const char*>(indices.data()),
        indices.size() * sizeof(unsigned int));

    return file.good();
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
            vertices[i].bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
        else
        {
            vertices[i].tangent = vertices[i].bitangent = glm::vec3(0.0f);
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
        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }
    return indices;
}