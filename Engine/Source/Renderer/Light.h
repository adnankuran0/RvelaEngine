#pragma once
#include "glm/glm.hpp"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/Components/SpotLightComponent.h"

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
    float normalBias;
    bool staticShadow = false;
    bool shadowDirty = false;
    uint32_t lightID = 0;
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
    bool staticShadow = false;
    bool shadowDirty = false;
    uint32_t lightID = 0;
};

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
    glm::mat4 lightSpace;
    glm::mat4 cascadeMatrices[NUM_SHADOW_CASCADES];
    float cascadeSplits[NUM_SHADOW_CASCADES];
    float intensity;
    bool castShadows;
    bool contactShadows = true;
    bool reverseCullFace;
    float blurRadius;
    float shadowBias;
    float normalBias;
    bool staticShadow = false;
    bool shadowDirty = false;
    uint32_t lightID = 0;
};

}
