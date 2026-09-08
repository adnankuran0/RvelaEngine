#pragma once
#include "Animation/IPropertyTrack.h"
#include "Animation/AnimationTrack.h"
#include "Scene/Components/SkeletonComponent.h"
#include "Asset/AssetManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace rv::Animation {

enum class BoneTransformType {
    Position,
    Rotation,
    Scale
};

template<typename T>
class BonePropertyTrack : public IPropertyTrack
{
public:
    AnimationTrack<T> track;
    BoneTransformType transformType = BoneTransformType::Rotation;
    mutable int cachedBoneIndex = -2;

    PropertyType GetType() const override;

    float GetLastKeyframeTime() const override
    {
        return track.keyframes.empty() ? 0.0f : track.keyframes.back().time;
    }

    void Apply(entt::registry& reg, entt::entity targetEntity, float time) override
    {
        if (track.keyframes.empty() || !reg.valid(targetEntity))
            return;

        if (!reg.all_of<SkeletonComponent>(targetEntity))
            return;

        auto& skel = reg.get<SkeletonComponent>(targetEntity);

        if (cachedBoneIndex == -2)
        {
            auto skeletonAsset = AssetManager::Get().GetAsset<SkeletonAsset>(skel.GetSkeletonID());
            if (skeletonAsset)
            {
                cachedBoneIndex = skeletonAsset->FindBoneIndex(targetPath);
            }
            else
            {
                cachedBoneIndex = -1;
            }
        }

        if (cachedBoneIndex < 0)
            return;

        T sampledValue = track.Sample(time);

        if constexpr (std::is_same_v<T, glm::vec3>)
        {
            if (transformType == BoneTransformType::Position)
                skel.localPositions[cachedBoneIndex] = sampledValue;
            else if (transformType == BoneTransformType::Scale)
                skel.localScales[cachedBoneIndex] = sampledValue;
        }
        else if constexpr (std::is_same_v<T, glm::quat>)
        {
            if (transformType == BoneTransformType::Rotation)
                skel.localRotations[cachedBoneIndex] = sampledValue;
        }
    }

    std::shared_ptr<IPropertyTrack> Clone() const override
    {
        auto cloned = std::make_shared<BonePropertyTrack<T>>();
        cloned->targetPath = targetPath;
        cloned->propertyName = propertyName;
        cloned->transformType = transformType;
        cloned->track = track;
        cloned->cachedBoneIndex = -2;
        return cloned;
    }
};

template<> inline PropertyType BonePropertyTrack<glm::vec3>::GetType() const { return PropertyType::BoneVec3; }
template<> inline PropertyType BonePropertyTrack<glm::quat>::GetType() const { return PropertyType::BoneQuat; }

}