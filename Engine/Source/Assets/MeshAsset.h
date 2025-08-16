#pragma once
#include "Assets/AssetMeta.h"
#include "Assets/Asset.h"
#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "Core/Log.h"
#include <fstream>
#include <span>
#include "Scene/AABB.h"

struct Vertex 
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec2 texCoord;
};

inline void PackVertices(std::span<const Vertex> verts, std::vector<float>& out)
{
    out.clear();
    out.reserve(verts.size() * 14);

    for (const auto& v : verts)
    {
        out.insert(out.end(), glm::value_ptr(v.position), glm::value_ptr(v.position) + 3);
        out.insert(out.end(), glm::value_ptr(v.normal), glm::value_ptr(v.normal) + 3);
        out.insert(out.end(), glm::value_ptr(v.tangent), glm::value_ptr(v.tangent) + 3);
        out.insert(out.end(), glm::value_ptr(v.bitangent), glm::value_ptr(v.bitangent) + 3);
        out.insert(out.end(), glm::value_ptr(v.texCoord), glm::value_ptr(v.texCoord) + 2);
    }
}

class MeshMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override
    {
        return std::make_unique<MeshMeta>(*this);
    }

    void Serialize(std::vector<char>& outBuffer, size_t& offset) const override
    {
        SerializeBase(outBuffer, offset); // UUID, sourcePath, type

        if (outBuffer.size() < offset + sizeof(vertexCount) + sizeof(indexCount))
            outBuffer.resize(offset + sizeof(vertexCount) + sizeof(indexCount));

        memcpy(outBuffer.data() + offset, &vertexCount, sizeof(vertexCount)); offset += sizeof(vertexCount);
        memcpy(outBuffer.data() + offset, &indexCount, sizeof(indexCount)); offset += sizeof(indexCount);
    }

    void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override
    {
        DeserializeBase(inBuffer,offset); // UUID, sourcePath, type
        memcpy(&vertexCount, inBuffer.data() + offset, sizeof(vertexCount)); offset += sizeof(vertexCount);
        memcpy(&indexCount, inBuffer.data() + offset, sizeof(indexCount)); offset += sizeof(indexCount);
    }

    

public:
    unsigned int vertexCount;
    unsigned int indexCount;
};

class MeshAsset : public Asset
{
public:
    MeshAsset(const std::string& path, std::unique_ptr<MeshMeta> meshMeta)
        :  Asset(std::move(meshMeta)), m_Path(path) {}
    inline bool IsLoaded() const { return m_Loaded; }

    bool Load()
    {
        if (m_Loaded) return true;

        MeshMeta* meta = GetMetaAs<MeshMeta>();
        if (!meta)
        {
            LOG_ERROR("Failed to get MeshMeta");
            return false;
        }

        std::ifstream file(m_Path, std::ios::binary);
        if (!file)
        {
            LOG_ERROR("Failed to open mesh file: {}", m_Path);
            return false;
        }

        AssetHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file || header.magic != MAGIC_MESH || header.type != AssetType::Mesh)
        {
            LOG_ERROR("Invalid mesh header in: {}", m_Path);
            return false;
        }

        file.seekg(sizeof(AssetHeader) + header.metaSize, std::ios::beg);
        file.read(reinterpret_cast<char*>(&localAABB), sizeof(AABB));

        vertices.resize(meta->vertexCount);
        indices.resize(meta->indexCount);

        file.read(reinterpret_cast<char*>(vertices.data()), sizeof(Vertex) * vertices.size());
        file.read(reinterpret_cast<char*>(indices.data()), sizeof(uint32_t) * indices.size());

        file.close();

        if (vertices.empty() || indices.empty())
        {
            LOG_ERROR("Vertex or index data missing in mesh: {}", m_Path);
            return false;
        }

        m_Loaded = true;
        return true;
    }

public:
    AABB localAABB;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    //AssetUUID materialUUID;
    std::string meshName;
    std::string m_Path;
private:
    bool m_Loaded = false;
};




