#include "rvelapch.h"
#include "CameraComponent.h"

namespace rv {

json CameraComponent::Serialize() const
{
    json j;
    j["fov"] = fov;
    j["nearClip"] = nearClip;
    j["farClip"] = farClip;
    j["isActive"] = isActive;
    return j;
}

void CameraComponent::Deserialize(const json& j)
{
    fov = j["fov"];
    nearClip = j["nearClip"];
    farClip = j["farClip"];
    isActive = j["isActive"];
}

}