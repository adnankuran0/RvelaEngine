#pragma once
#include "glm/glm.hpp"
#include "../nlohmann/json.hpp"

namespace rv {

using json = nlohmann::json;

struct PointLightComponent 
{
public:

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 20.0f;
    float radius = 5.0f;
    float falloff = 2.0f;
    bool castShadows = false;
    bool reverseCullFace = true;
    float shadowBias = 0.1f;
    float blurRadius = 0.03f;
    PointLightComponent() = default;
    PointLightComponent(const glm::vec3& color, float intensity, float radius)
        : color(color), intensity(intensity), radius(radius) {}

    json Serialize() const;
    void Deserialize(const json& j);
 
};


}
