#pragma once
#include "Scene/Component.h"
#include "glm/glm.hpp"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

class DirectionalLightComponent : public Component
{
public:
    DirectionalLightComponent() = default;
    DirectionalLightComponent(const glm::vec3& color, float intensity) : color(color), intensity(intensity) {}

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;
    bool castShadows = true;
    float shadowBias = 0.005f;

    std::string Serialize() const override
    {
        json j;
        j["color"] = { color.r,color.g,color.b };
        j["intensity"] = intensity;
        j["castShadows"] = castShadows;
        return j.dump(4);
    }

    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto colorData = j["color"];
        color = glm::vec3(colorData[0], colorData[1], colorData[2]);
        intensity = j["intensity"];
        castShadows = j["castShadows"];
    }
};
