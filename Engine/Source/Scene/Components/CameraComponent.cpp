#include "rvelapch.h"
#include "CameraComponent.h"

using namespace rv;

json CameraComponent::Serialize() const
{
    json j;
    j["fov"] = camera.FOV;
    j["nearClip"] = camera.NearClip;
    j["farClip"] = camera.FarClip;
    j["projectionType"] = static_cast<int>(camera.ProjectionType);
    j["isActive"] = isActive;
    return j;
}

void CameraComponent::Deserialize(const json& j)
{
    camera.FOV = j["fov"];
    camera.NearClip = j["nearClip"];
    camera.FarClip = j["farClip"];
    //camera.ProjectionType = static_cast<Camera::Projection>(j["projectionType"]);
    isActive = j["isActive"];
}
