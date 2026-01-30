#pragma once
#include "glm/glm.hpp"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

struct DirectionalLightComponent 
{
public:
    DirectionalLightComponent() = default;
    DirectionalLightComponent(const glm::vec3& color, float intensity) : color(color), intensity(intensity) {}

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 5.0f;
    float shadowBias = 0.001f;
    float blurRadius = 0.5f;
    bool castShadows = true;
    bool reverseCullFace = true;

    json Serialize() const;
    void Deserialize(const json& j);
};
