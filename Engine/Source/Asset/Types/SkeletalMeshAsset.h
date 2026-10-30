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

    struct BoneBound
    {
        AABB aabb;
        bool hasVertices = false;
    };

    const std::vector<BoneBound>& GetBoneBounds() const;
    AABB CalculateAnimatedAABB(const std::vector<glm::mat4>& skinningPalette) const;
    void ComputeBoneBounds();

    void CopyFrom(const SkeletalMeshAsset& other)
    {
        m_Vertices = other.m_Vertices;
        m_Indices = other.m_Indices;
        m_LocalAABB = other.m_LocalAABB;
        m_MeshName = other.m_MeshName;
        m_BoneBounds = other.m_BoneBounds;
        m_UnweightedBounds = other.m_UnweightedBounds;
        m_HasUnweighted = other.m_HasUnweighted;
        m_BoneBoundsComputed = other.m_BoneBoundsComputed;
    }

private:
    friend class SkeletalMeshLoader;
    friend class AssetManager;

    std::vector<SkeletalVertex> m_Vertices;
    std::vector<unsigned int>   m_Indices;
    AABB m_LocalAABB;
    std::string m_MeshName;

    std::vector<BoneBound> m_BoneBounds;
    AABB m_UnweightedBounds;
    bool m_HasUnweighted = false;
    bool m_BoneBoundsComputed = false;
};

}