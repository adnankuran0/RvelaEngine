#include "rvelapch.h"
#include "DirectionalLightComponent.h"

std::string DirectionalLightComponent::Serialize() const
{
    json j;
    j["color"] = { color.r,color.g,color.b };
    j["intensity"] = intensity;
    j["castShadows"] = castShadows;
    return j.dump(4);
}

void DirectionalLightComponent::Deserialize(const std::string& jsonStr)
{
    json j = json::parse(jsonStr);
    auto colorData = j["color"];
    color = glm::vec3(colorData[0], colorData[1], colorData[2]);
    intensity = j["intensity"];
}