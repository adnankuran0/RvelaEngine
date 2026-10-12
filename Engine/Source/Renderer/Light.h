#pragma once
#include "glm/glm.hpp"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/Components/SpotLightComponent.h"

namespace rv {

inline constexpr int NUM_SHADOW_CASCADES = 4;

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
    float normalBias;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float radius;
    float innerCutoff;
    float outerCutoff;
    bool castShadows;
    int shadowIndex;
    bool reverseCullFace;
    float blurRadius;
    float shadowBias;
    float normalBias;
    glm::mat4 lightSpaceMatrix;
};

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
    glm::mat4 lightSpace;
    glm::mat4 cascadeMatrices[NUM_SHADOW_CASCADES];
    float cascadeSplits[NUM_SHADOW_CASCADES];
    float intensity;
    bool castShadows;
    bool reverseCullFace;
    float blurRadius;
    float shadowBias;
    float normalBias;
};

}

