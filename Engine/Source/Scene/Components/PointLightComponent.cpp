#include "rvelapch.h"
#include "PointLightComponent.h"

namespace rv {

json PointLightComponent::Serialize() const
{
    json j;
    j["color"] = { color.r,color.g,color.b };
    j["intensity"] = intensity;
    j["radius"] = radius;
    j["falloff"] = falloff;
    j["castShadows"] = castShadows;
    return j;
}

void PointLightComponent::Deserialize(const json& j)
{
    auto colorData = j.at("color");
    color = glm::vec3(colorData[0], colorData[1], colorData[2]);
    intensity = j.at("intensity").get<float>();
    radius = j.at("radius").get<float>();
    falloff = j.at("falloff").get<float>();
    castShadows = j.at("castShadows").get<float>();
}

}