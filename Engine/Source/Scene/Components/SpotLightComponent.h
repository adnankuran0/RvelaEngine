#pragma once
#include "glm/glm.hpp"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;
struct SpotLightComponent 
{
public:
    SpotLightComponent() = default;
    SpotLightComponent(const glm::vec3& color, float intensity, float radius, float innerCutoff, float outerCutoff)
        : color(color), intensity(intensity), radius(radius), innerCutoff(glm::cos(glm::radians(innerCutoff))), outerCutoff(glm::cos(glm::radians(outerCutoff))) {
    }

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float radius = 5.0f;
    float innerCutoff = glm::cos(glm::radians(15.0f));
    float outerCutoff = glm::cos(glm::radians(30.0f));

    std::string Serialize() const;
    void Deserialize(const std::string& jsonStr);

};