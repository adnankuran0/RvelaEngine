#include "rvelapch.h"
#include "SpotLightComponent.h"

namespace rv {

std::string SpotLightComponent::Serialize() const
{
    json j;
    j["color"] = { color.r,color.g,color.b };
    j["intensity"] = intensity;
    j["radius"] = radius;
    j["innerCutoff"] = innerCutoff;
    j["outerCutoff"] = outerCutoff;
    return j.dump(4);
}

void SpotLightComponent::Deserialize(const json& j)
{
    auto colorData = j.at("color");
    color = glm::vec3(colorData[0], colorData[1], colorData[2]);
    intensity = j["intensity"];
    radius = j["radius"];
    innerCutoff = j["innerCutoff"];
    outerCutoff = j["outerCutoff"];
}

}