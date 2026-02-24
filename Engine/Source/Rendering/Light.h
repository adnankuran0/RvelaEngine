#pragma once
#include "glm/glm.hpp"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"

namespace rv {

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
    float falloff;
    bool castShadows;
    int shadowIndex;
    bool reverseCullFace;
    float blurRadius;
    float shadowBias;
};

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
    glm::mat4 lightSpace;
    float intensity;
    bool castShadows;
    bool reverseCullFace;
    float blurRadius;
    float shadowBias;
};

}

