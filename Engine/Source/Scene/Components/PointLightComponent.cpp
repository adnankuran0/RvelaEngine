#include "rvelapch.h"
#include "PointLightComponent.h"

using namespace rv;

json PointLightComponent::Serialize() const
{
    json j;
    j["color"] = { color.r, color.g, color.b };
    j["intensity"] = intensity;
    j["radius"] = radius;
    j["falloff"] = falloff;
    j["castShadows"] = castShadows;
    j["reverseCullFace"] = reverseCullFace;
    j["shadowBias"] = shadowBias;
    j["blurRadius"] = blurRadius;
    return j;
}

void PointLightComponent::Deserialize(const json& j)
{
    auto colorData = j.at("color");
    color = glm::vec3(colorData[0], colorData[1], colorData[2]);
    intensity = j.at("intensity").get<float>();
    radius = j.at("radius").get<float>();
    falloff = j.at("falloff").get<float>();
    castShadows = j.at("castShadows").get<bool>();
    if (j.contains("reverseCullFace")) reverseCullFace = j.at("reverseCullFace").get<bool>();
    if (j.contains("shadowBias")) shadowBias = j.at("shadowBias").get<float>();
    if (j.contains("blurRadius")) blurRadius = j.at("blurRadius").get<float>();
}
