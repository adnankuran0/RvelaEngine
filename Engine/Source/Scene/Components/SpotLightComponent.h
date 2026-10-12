#pragma once
#include "glm/glm.hpp"
#include "../nlohmann/json.hpp"

namespace rv {

using json = nlohmann::json;
struct SpotLightComponent 
{
public:
    SpotLightComponent() = default;
    SpotLightComponent(const glm::vec3& color, float intensity, float radius, float innerAngle, float outerAngle)
        : color(color), intensity(intensity), radius(radius), innerAngle(innerAngle), outerAngle(outerAngle),
          innerCutoff(glm::cos(glm::radians(innerAngle))), outerCutoff(glm::cos(glm::radians(outerAngle))) {
    }

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;
    float radius = 5.0f;
    float innerAngle = 5.0f;
    float outerAngle = 25.0f;
    float innerCutoff = glm::cos(glm::radians(15.0f));
    float outerCutoff = glm::cos(glm::radians(30.0f));

    bool castShadows = true;
    float shadowBias = 0.005f;
    float blurRadius = 1.0f;
    bool reverseCullFace = false;

    std::string Serialize() const;
    void Deserialize(const json& j);

    void UpdateCutoffs()
    {
        if (innerAngle > outerAngle)
            innerAngle = outerAngle;
        innerCutoff = glm::cos(glm::radians(innerAngle));
        outerCutoff = glm::cos(glm::radians(outerAngle));
    }
};

}