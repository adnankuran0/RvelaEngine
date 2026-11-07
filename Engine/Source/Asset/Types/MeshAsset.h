#pragma once
#include "Asset/Asset.h"
#include "Renderer/AABB.h"
#include <span>
#include <vector>
#include <glm/glm.hpp>

namespace rv {

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

void PackVertices(std::span<const Vertex> verts, std::vector<float>& out);

struct MeshLODInfo
{
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    float screenRatio = 0.0f;
    float lodError = 0.0f;
};

class MeshAsset : public Asset
{
public:
    explicit MeshAsset(AssetUUID uuid) : Asset(uuid) {}

    std::span<const Vertex> GetVertices() const { return m_Vertices; }

    std::span<const unsigned int> GetAllIndices() const { return m_Indices; }
    std::span<const unsigned int> GetIndices() const { return GetLODIndices(0); }
    std::span<const unsigned int> GetLODIndices(uint32_t lodIndex = 0) const
    {
        if (lodIndex < m_LODs.size())
        {
            const auto& lod = m_LODs[lodIndex];
            return std::span<const unsigned int>(m_Indices.data() + lod.indexOffset, lod.indexCount);
        }
        return m_Indices;
    }

    std::span<const MeshLODInfo> GetLODs() const { return m_LODs; }
    uint32_t GetLODCount() const { return static_cast<uint32_t>(m_LODs.size()); }
    const MeshLODInfo& GetLOD(uint32_t index) const { return m_LODs[index]; }

    const AABB& GetAABB() const { return m_LocalAABB; }
    const std::string& GetMeshName() const { return m_MeshName; }

    uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_Vertices.size()); }
    uint32_t GetAllIndexCount() const { return static_cast<uint32_t>(m_Indices.size()); }
    uint32_t GetIndexCount(uint32_t lodIndex = 0) const
    {
        if (lodIndex < m_LODs.size())
            return m_LODs[lodIndex].indexCount;
        return static_cast<uint32_t>(m_Indices.size());
    }
    uint32_t GetTriangleCount(uint32_t lodIndex = 0) const
    {
        return GetIndexCount(lodIndex) / 3;
    }

    bool IsValid() const { return !m_Vertices.empty() && !m_Indices.empty(); }

    void GetTriangle(int triangleIndex, glm::vec3& v0, glm::vec3& v1, glm::vec3& v2) const;

    void CopyFrom(const MeshAsset& other)
    {
        m_Vertices = other.m_Vertices;
        m_Indices = other.m_Indices;
        m_LODs = other.m_LODs;
        m_LocalAABB = other.m_LocalAABB;
        m_MeshName = other.m_MeshName;
    }

private:
    friend class MeshLoader;
    friend class AssetManager;

    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;
    std::vector<MeshLODInfo> m_LODs;
    AABB m_LocalAABB;
    std::string m_MeshName;
};

}
