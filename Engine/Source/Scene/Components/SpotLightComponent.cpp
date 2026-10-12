#include "rvelapch.h"
#include "SpotLightComponent.h"

using namespace rv;

std::string SpotLightComponent::Serialize() const
{
    json j;
    j["color"] = { color.r, color.g, color.b };
    j["intensity"] = intensity;
    j["radius"] = radius;
    j["innerAngle"] = innerAngle;
    j["outerAngle"] = outerAngle;
    j["innerCutoff"] = innerCutoff;
    j["outerCutoff"] = outerCutoff;
    j["castShadows"] = castShadows;
    j["shadowBias"] = shadowBias;
    j["blurRadius"] = blurRadius;
    j["reverseCullFace"] = reverseCullFace;
    return j.dump(4);
}

void SpotLightComponent::Deserialize(const json& j)
{
    if (j.contains("color"))
    {
        auto colorData = j.at("color");
        color = glm::vec3(colorData[0], colorData[1], colorData[2]);
    }
    intensity = j.value("intensity", 1.0f);
    radius = j.value("radius", 15.0f);
    innerAngle = j.value("innerAngle", 15.0f);
    outerAngle = j.value("outerAngle", 30.0f);
    innerCutoff = j.value("innerCutoff", glm::cos(glm::radians(innerAngle)));
    outerCutoff = j.value("outerCutoff", glm::cos(glm::radians(outerAngle)));
    castShadows = j.value("castShadows", true);
    shadowBias = j.value("shadowBias", 0.005f);
    blurRadius = j.value("blurRadius", 1.0f);
    reverseCullFace = j.value("reverseCullFace", false);
}
