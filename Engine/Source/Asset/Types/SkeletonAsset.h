#pragma once
#include "Asset/Asset.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace rv {

class SkeletonAsset : public Asset
{
public:
    explicit SkeletonAsset(AssetUUID uuid) : Asset(uuid) {}

    uint32_t GetBoneCount() const { return (uint32_t)m_ParentIndices.size(); }
    bool IsValid() const { return !m_ParentIndices.empty(); }

    int32_t GetParentIndex(uint32_t boneIndex) const { return m_ParentIndices[boneIndex]; }
    const std::string& GetBoneName(uint32_t boneIndex) const { return m_BoneNames[boneIndex]; }

    int32_t FindBoneIndex(const std::string& name) const
    {
        auto it = m_BoneNameToIndex.find(name);
        return it != m_BoneNameToIndex.end() ? it->second : -1;
    }

    const glm::vec3& GetBindPosition(uint32_t i) const { return m_BindPositions[i]; }
    const glm::quat& GetBindRotation(uint32_t i) const { return m_BindRotations[i]; }
    const glm::vec3& GetBindScale(uint32_t i) const { return m_BindScales[i]; }
    const glm::mat4& GetInverseBindMatrix(uint32_t i) const { return m_InverseBindMatrices[i]; }

private:
    friend class SkeletonLoader;
    friend class SkeletonSerializer;
    friend class SkeletonImporter;

    std::vector<int32_t> m_ParentIndices;
    std::vector<std::string> m_BoneNames;
    std::unordered_map<std::string, int32_t> m_BoneNameToIndex;

    std::vector<glm::vec3> m_BindPositions;
    std::vector<glm::quat> m_BindRotations;
    std::vector<glm::vec3> m_BindScales;

    std::vector<glm::mat4> m_InverseBindMatrices;
};

}