#pragma once
#include "Scene/Component.h"
#include "glm/glm.hpp"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;



class PointLightComponent : public Component
{
public:

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;
    float radius = 10.0f;
    float falloff = 0.0f;
    bool castShadows = true;
    int shadowIndex = -1;
    PointLightComponent()
    {
        shadowIndex = s_ShadowIndexCounter;
        s_ShadowIndexCounter++;
    }
    PointLightComponent(const glm::vec3& color, float intensity, float radius) : color(color), intensity(intensity), radius(radius) 
    {
        shadowIndex = s_ShadowIndexCounter;
        s_ShadowIndexCounter++;
    }

    std::string Serialize() const override
    {
        json j;
        j["color"] = { color.r,color.g,color.b };
        j["intensity"] = intensity;
        j["radius"] = radius;
        return j.dump(4);
    }

    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto colorData = j["color"];
        color = glm::vec3(colorData[0], colorData[1], colorData[2]);
        intensity = j["intensity"];
        radius = j["radius"];
    }

private:
    static int s_ShadowIndexCounter;

};

