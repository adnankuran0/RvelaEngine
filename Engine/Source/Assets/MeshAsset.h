#pragma once
#include "Assets/AssetMeta.h"
#include "Assets/Asset.h"
#include "glm/glm.hpp"
#include "Core/Log.h"
#include <fstream>

struct Vertex 
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec2 texCoord;
};

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
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    //AssetUUID materialUUID;
    std::string meshName;
    std::string m_Path;
private:
    bool m_Loaded = false;
};




#if 0

struct MeshComponent
{
    Ref<MeshAsset> mesh;
};

class alignas(16) MeshRendererComponent {
public:
    BufferLayout layout;
    BoundingBox localAABB;
    BoundingBox worldAABB;
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
    unsigned int indexCount = 0;

    MeshRendererComponent() = default;
    MeshRendererComponent(const MeshRendererComponent&) = delete;
    MeshRendererComponent& operator=(const MeshRendererComponent&) = delete;
    MeshRendererComponent(MeshRendererComponent&&) = default;
    MeshRendererComponent& operator=(MeshRendererComponent&&) = default;
    MeshRendererComponent(void* vertices, size_t sizeOfVertices, void* indices, size_t sizeOfIndices, unsigned int indexCount, BoundingBox localAABB)
    {
        this->indexCount = indexCount;

        VAO.Bind();
        VBO.Init(vertices, sizeOfVertices);
        VBO.Bind();
        layout.BindVertexBuffer(VBO.getID());
        layout.Push<float>(3); // Position
        layout.Push<float>(3); // Normal
        layout.Push<float>(3); // Tangent
        layout.Push<float>(3); // Bitangent
        layout.Push<float>(2); // UV
        VAO.SetBufferLayout(layout);
        EBO.Init(indices, sizeOfIndices);
        EBO.Bind();
        this->localAABB = localAABB;
    }

    void Destroy()
    {
        VAO.Destroy();
        VBO.Destroy();
        EBO.Destroy();
    }
};

#endif