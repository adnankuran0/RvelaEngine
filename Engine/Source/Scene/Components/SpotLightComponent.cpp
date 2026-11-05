#include "rvelapch.h"
#include "SpotLightComponent.h"

using namespace rv;

json SpotLightComponent::Serialize() const
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
    j["staticShadow"] = staticShadow;
    return j;
}

void SpotLightComponent::Deserialize(const json& j)
{
    const json* source = &j;
    json parsed;
    if (j.is_string())
    {
        try {
            parsed = json::parse(j.get<std::string>());
            source = &parsed;
        } catch (...) {
            return;
        }
    }

    const json& src = *source;
    if (!src.is_object())
        return;

    if (src.contains("color"))
    {
        auto colorData = src.at("color");
        color = glm::vec3(colorData[0], colorData[1], colorData[2]);
    }
    intensity = src.value("intensity", 10.0f);
    radius = src.value("radius", 5.0f);
    innerAngle = src.value("innerAngle", 5.0f);
    outerAngle = src.value("outerAngle", 25.0f);
    innerCutoff = src.value("innerCutoff", glm::cos(glm::radians(innerAngle)));
    outerCutoff = src.value("outerCutoff", glm::cos(glm::radians(outerAngle)));
    castShadows = src.value("castShadows", true);
    shadowBias = src.value("shadowBias", 0.005f);
    blurRadius = src.value("blurRadius", 1.0f);
    reverseCullFace = src.value("reverseCullFace", false);
    staticShadow = src.value("staticShadow", false);
    shadowDirty = true;
}
