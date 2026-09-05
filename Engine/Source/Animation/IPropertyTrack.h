#pragma once
#include "Animation/AnimationTrack.h"
#include "Animation/PropertyBindingRegistry.h"
#include <string>

namespace rv::Animation {

enum class PropertyType { Float, Vec3, Vec4, Quat, Bool };

class IPropertyTrack 
{
public:
    std::string targetPath;
    std::string propertyName;

    virtual ~IPropertyTrack() = default;
    virtual PropertyType GetType() const = 0;
    virtual float GetLastKeyframeTime() const = 0;
    virtual void Apply(entt::registry& reg, entt::entity targetEntity, float time) = 0;
    virtual std::shared_ptr<IPropertyTrack> Clone() const = 0;
};

template<typename T>
class TypedPropertyTrack : public IPropertyTrack 
{
public:
    AnimationTrack<T> track;

    PropertyType GetType() const override;

    float GetLastKeyframeTime() const override 
    {
        return track.keyframes.empty() ? 0.0f : track.keyframes.back().time;
    }

    void Apply(entt::registry& reg, entt::entity targetEntity, float time) override 
    {
        if (track.keyframes.empty()) return;
        T sampledValue = track.Sample(time);
        PropertyBindingRegistry::Get().Apply(reg, targetEntity, propertyName, sampledValue);
    }

    std::shared_ptr<IPropertyTrack> Clone() const override 
    {
        auto cloned = std::make_shared<TypedPropertyTrack<T>>();
        cloned->targetPath = targetPath;
        cloned->propertyName = propertyName;
        cloned->track = track;
        return cloned;
    }
};

template<> inline PropertyType TypedPropertyTrack<float>::GetType() const { return PropertyType::Float; }
template<> inline PropertyType TypedPropertyTrack<glm::vec3>::GetType() const { return PropertyType::Vec3; }
template<> inline PropertyType TypedPropertyTrack<glm::vec4>::GetType() const { return PropertyType::Vec4; }
template<> inline PropertyType TypedPropertyTrack<glm::quat>::GetType() const { return PropertyType::Quat; }
template<> inline PropertyType TypedPropertyTrack<bool>::GetType() const { return PropertyType::Bool; }

}