#pragma once
#include "glm/glm.hpp"
#include "../nlohmann/json.hpp"

namespace rv {

using json = nlohmann::json;

struct DirectionalLightComponent 
{
public:
    DirectionalLightComponent() = default;
    DirectionalLightComponent(const glm::vec3& color, float intensity) : color(color), intensity(intensity) {}

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 5.0f;
    float shadowBias = 0.0003f;
    float normalBias = 0.008f;
    float blurRadius = 1.0f;
    bool castShadows = true;
    bool reverseCullFace = false;

    json Serialize() const;
    void Deserialize(const json& j);
};

}
