#include "rvelapch.h"
#include "DirectionalLightComponent.h"

using namespace rv;

json DirectionalLightComponent::Serialize() const
{
    json j;
    j["color"] = { color.r,color.g,color.b };
    j["intensity"] = intensity;
    j["castShadows"] = castShadows;
    j["reverseCullFace"] = reverseCullFace;
    return j;
}

void DirectionalLightComponent::Deserialize(const json& j)
{
    auto colorData = j.at("color");
    color = glm::vec3(colorData[0], colorData[1], colorData[2]);
    intensity = j["intensity"];
    castShadows = j["castShadows"];
    reverseCullFace = j["reverseCullFace"];
}
