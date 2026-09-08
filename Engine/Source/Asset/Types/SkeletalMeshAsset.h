#pragma once
#include "Asset/Asset.h"
#include "Renderer/AABB.h"
#include <span>
#include <glm/glm.hpp>

namespace rv {

struct SkeletalVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
    uint32_t boneIDs[4];
    float weights[4];
};

void PackVertices(std::span<const SkeletalVertex> verts, std::vector<float>& out);

class SkeletalMeshAsset : public Asset
{
public:
    explicit SkeletalMeshAsset(AssetUUID uuid) : Asset(uuid) {}

    std::span<const SkeletalVertex> GetVertices() const { return m_Vertices; }
    std::span<const unsigned int>   GetIndices() const { return m_Indices; }
    const AABB& GetAABB() const { return m_LocalAABB; }
    const std::string& GetMeshName() const { return m_MeshName; }

    uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_Vertices.size()); }
    uint32_t GetIndexCount() const { return static_cast<uint32_t>(m_Indices.size()); }
    uint32_t GetTriangleCount() const { return static_cast<uint32_t>(m_Indices.size()) / 3; }

    bool IsValid() const { return !m_Vertices.empty() && !m_Indices.empty(); }

private:
    friend class SkeletalMeshLoader;

    std::vector<SkeletalVertex> m_Vertices;
    std::vector<unsigned int>   m_Indices;
    AABB m_LocalAABB;
    std::string m_MeshName;
};

}