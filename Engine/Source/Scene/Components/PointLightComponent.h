#pragma once
#include "glm/glm.hpp"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

struct PointLightComponent 
{
public:

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;
    float radius = 5.0f;
    float falloff = 0.0f;
    bool castShadows = false;
    bool reverseCullFace = true;
    float shadowBias = 0.1f;
    float blurRadius = 0.03f;
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

    json Serialize() const;
    void Deserialize(const json& j);
 

private:
    inline static int s_ShadowIndexCounter = 0;

};

