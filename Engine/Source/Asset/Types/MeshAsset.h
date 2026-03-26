#pragma once
#include "Asset/Asset.h"
#include "Rendering/AABB.h"
#include <span>
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

class MeshAsset : public Asset
{
public:
    explicit MeshAsset(AssetUUID uuid) : Asset(uuid) {}

    std::span<const Vertex> GetVertices() const { return m_Vertices; }
    std::span<const unsigned int> GetIndices() const { return m_Indices; }
    const AABB& GetAABB() const { return m_LocalAABB; }
    const std::string& GetMeshName() const { return m_MeshName; }

    uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_Vertices.size()); }
    uint32_t GetIndexCount() const { return static_cast<uint32_t>(m_Indices.size()); }
    uint32_t GetTriangleCount() const { return static_cast<uint32_t>(m_Indices.size()) / 3; }

    bool IsValid() const { return !m_Vertices.empty() && !m_Indices.empty(); }

    void GetTriangle(int triangleIndex,glm::vec3& v0, glm::vec3& v1, glm::vec3& v2) const;

private:
    friend class MeshLoader;

    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;
    AABB m_LocalAABB;
    std::string m_MeshName;
};

}