#include "rvelapch.h"
#include "PointLightComponent.h"

std::string PointLightComponent::Serialize() const
{
    json j;
    j["color"] = { color.r,color.g,color.b };
    j["intensity"] = intensity;
    j["radius"] = radius;
    j["falloff"] = falloff;
    j["castShadows"] = castShadows;
    return j.dump(4);
}

void PointLightComponent::Deserialize(const std::string& jsonStr)
{
    json j = json::parse(jsonStr);
    auto colorData = j["color"];
    color = glm::vec3(colorData[0], colorData[1], colorData[2]);
    intensity = j["intensity"];
    radius = j["radius"];
    falloff = j["falloff"];
    castShadows = j["castShadows"];
}