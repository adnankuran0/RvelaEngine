#include "rvelapch.h"
#include "SkeletalMeshAsset.h"
#include <glm/gtc/type_ptr.hpp>

using namespace rv;

void rv::PackVertices(std::span<const SkeletalVertex> verts, std::vector<float>& out)
{
    out.clear();
    out.reserve(verts.size() * 19);

    for (const auto& v : verts)
    {
        out.insert(out.end(), glm::value_ptr(v.position), glm::value_ptr(v.position) + 3);
        out.insert(out.end(), glm::value_ptr(v.normal), glm::value_ptr(v.normal) + 3);
        out.insert(out.end(), glm::value_ptr(v.tangent), glm::value_ptr(v.tangent) + 3);

        out.insert(out.end(), glm::value_ptr(v.texCoord), glm::value_ptr(v.texCoord) + 2);

        for (int i = 0; i < 4; i++)
        {
            out.push_back(static_cast<float>(v.boneIDs[i]));
        }

        for (int i = 0; i < 4; i++)
        {
            out.push_back(v.weights[i]);
        }
    }
}

const std::vector<SkeletalMeshAsset::BoneBound>& SkeletalMeshAsset::GetBoneBounds() const
{
    if (!m_BoneBoundsComputed && !m_Vertices.empty())
    {
        const_cast<SkeletalMeshAsset*>(this)->ComputeBoneBounds();
    }
    return m_BoneBounds;
}

void SkeletalMeshAsset::ComputeBoneBounds()
{
    m_BoneBounds.clear();
    m_UnweightedBounds.Reset();
    m_HasUnweighted = false;

    for (const auto& v : m_Vertices)
    {
        bool hasWeight = false;
        for (int i = 0; i < 4; ++i)
        {
            if (v.weights[i] > 0.001f)
            {
                hasWeight = true;
                uint32_t bId = v.boneIDs[i];
                if (bId >= m_BoneBounds.size())
                {
                    m_BoneBounds.resize(bId + 1);
                }
                m_BoneBounds[bId].aabb.Expand(v.position);
                m_BoneBounds[bId].hasVertices = true;
            }
        }

        if (!hasWeight)
        {
            m_UnweightedBounds.Expand(v.position);
            m_HasUnweighted = true;
        }
    }

    m_BoneBoundsComputed = true;
}

AABB SkeletalMeshAsset::CalculateAnimatedAABB(const std::vector<glm::mat4>& skinningPalette) const
{
    const auto& boneBounds = GetBoneBounds();
    if (boneBounds.empty() || skinningPalette.empty())
    {
        return m_LocalAABB;
    }

    AABB animatedAABB;
    const size_t count = std::min(boneBounds.size(), skinningPalette.size());
    for (size_t i = 0; i < count; ++i)
    {
        const auto& bb = boneBounds[i];
        if (bb.hasVertices && bb.aabb.IsValid())
        {
            const auto& pal = skinningPalette[i];
            const glm::vec3 corners[8] = {
                {bb.aabb.min.x, bb.aabb.min.y, bb.aabb.min.z},
                {bb.aabb.max.x, bb.aabb.min.y, bb.aabb.min.z},
                {bb.aabb.min.x, bb.aabb.max.y, bb.aabb.min.z},
                {bb.aabb.max.x, bb.aabb.max.y, bb.aabb.min.z},
                {bb.aabb.min.x, bb.aabb.min.y, bb.aabb.max.z},
                {bb.aabb.max.x, bb.aabb.min.y, bb.aabb.max.z},
                {bb.aabb.min.x, bb.aabb.max.y, bb.aabb.max.z},
                {bb.aabb.max.x, bb.aabb.max.y, bb.aabb.max.z}
            };

            for (int c = 0; c < 8; ++c)
            {
                glm::vec4 p = pal * glm::vec4(corners[c], 1.0f);
                animatedAABB.Expand(glm::vec3(p));
            }
        }
    }

    if (m_HasUnweighted && m_UnweightedBounds.IsValid())
    {
        animatedAABB.Expand(m_UnweightedBounds.min);
        animatedAABB.Expand(m_UnweightedBounds.max);
    }

    if (!animatedAABB.IsValid())
    {
        return m_LocalAABB;
    }

    return animatedAABB;
}