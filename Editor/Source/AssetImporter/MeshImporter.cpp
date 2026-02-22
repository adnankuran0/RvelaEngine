#include "rvelapch.h"

#include "MeshImporter.h"

#include "Scene/AABB.h"

#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"
#include <Core/Log.h>

using namespace rv;

static std::string SanitizeFilename(const std::string& name)
{
    std::string result = name;
    for (char& c : result)
    {
        if (c == ':' || c == '/' || c == '\\' || c == '?' || c == '*' ||
            c == '"' || c == '<' || c == '>' || c == '|')
        {
            c = '_';
        }
    }
    return result;
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

        if (hasNormals)
            vertices[i].normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        else
            vertices[i].normal = glm::vec3(0.0f);

        if (hasTangents)
        {
            vertices[i].tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertices[i].bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
        else
        {
            vertices[i].tangent = glm::vec3(0.0f);
            vertices[i].bitangent = glm::vec3(0.0f);
        }

        if (hasTexCoords)
            vertices[i].texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertices[i].texCoord = glm::vec2(0.0f);
    }

    return vertices;
}

std::vector<unsigned int> MeshImporter::ProcessIndices(aiMesh* mesh)
{
    if (!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
    {
        std::cout << "Mesh is not triangle type." << std::endl;
    }

    std::vector<unsigned int> indices;
    indices.reserve(mesh->mNumFaces * 3);

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        assert(face.mNumIndices == 3);
        indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }

    return indices;
}

MeshMeta MeshImporter::CreateMeshMeta(const std::filesystem::path& path, aiMesh* mesh)
{
    MeshMeta meta;
    meta.sourcePath = path;
    meta.type = AssetType::Mesh;
    meta.vertexCount = mesh->mNumVertices;
    meta.indexCount = mesh->mNumFaces * 3;
    return meta;
}

bool MeshImporter::Import(const aiScene* scene, const std::filesystem::path& path, std::unordered_map<unsigned int, AssetUUID>& meshMap)
{

    if (scene->mNumMeshes == 0)
    {
        LOG_WARN("No meshes found in file: {}", path.string());
        return false;
    }

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
    {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        std::vector<Vertex> vertices = ProcessVertices(mesh);
        std::vector<unsigned int> indices = ProcessIndices(mesh);
        MeshMeta meta = CreateMeshMeta(path, mesh);
        LOG_INFO("Mesh loaded with UUID: {}", meta.uuid.ToString());
        meshMap[meshIndex] = meta.uuid;

        std::vector<char> metaBuffer;
        size_t offset = 0;
        meta.Serialize(metaBuffer, offset);

        AssetHeader header;
        header.magic = MAGIC_MESH;
        header.version = 1;
        header.type = AssetType::Mesh;
        header.metaSize = static_cast<uint32_t>(metaBuffer.size());
        header.dataOffset = sizeof(AssetHeader) + header.metaSize;
        header.reserved = 0;

        std::filesystem::path outputPath = path;
        std::string meshName = mesh->mName.C_Str();
        if (meshName.empty() || std::all_of(meshName.begin(), meshName.end(), isspace))
        {
            meshName = "Mesh" + std::to_string(meshIndex);
        }
        meshName = SanitizeFilename(meshName);

        aiVector3D min = mesh->mAABB.mMin;
        aiVector3D max = mesh->mAABB.mMax;
        AABB localAABB(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, max.y, max.z));

        outputPath.replace_filename(outputPath.stem().string() + "_" + meshName + ".rmesh");

        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile)
        {
            LOG_ERROR("Failed to open output file: {}", outputPath.string());
            continue;
        }

        outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
        outFile.write(metaBuffer.data(), metaBuffer.size());
        outFile.write(reinterpret_cast<const char*>(&localAABB), sizeof(AABB));
        outFile.write(reinterpret_cast<const char*>(vertices.data()), vertices.size() * sizeof(Vertex));
        outFile.write(reinterpret_cast<const char*>(indices.data()), indices.size() * sizeof(unsigned int));
        outFile.close();

        LOG_INFO("Mesh [{}] imported to: {} ({} vertices, {} indices) UUID: {}",
            meshName, outputPath.string(), vertices.size(), indices.size(), meta.uuid.ToString());
    }

    return true;
}

