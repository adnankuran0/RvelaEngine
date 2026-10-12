#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL

#define MAX_POINT_LIGHTS 20
#define MAX_SPOT_LIGHTS 8
#define NUM_CASCADES 4

struct PointLight {
    vec4 position;       // xyz: pos, w: falloff
    vec4 colorIntensity; // rgb: color, w: intensity
    float radius;
    float shadowBias;
    float blurRadius;
    int shadowIndex;
};

struct SpotLight {
    vec4 position;          // xyz: pos, w: radius
    vec4 direction;         // xyz: dir, w: shadowIndex
    vec4 colorIntensity;    // rgb: color, w: intensity
    vec4 cutoffs;           // x: innerCutoff, y: outerCutoff, z: shadowBias, w: blurRadius
    mat4 lightSpaceMatrix;  // for shadow mapping
};

struct DirectionalLight {
    vec4 direction;      // xyz: dir, w: castShadows
    vec4 colorIntensity; // rgb: color, w: intensity
    float shadowBias;
    float blurRadius;
    float normalBias;
    int cascadeCount;
};

layout(std140, binding = 1) uniform LightData {
    DirectionalLight directionalLight;
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight spotLights[MAX_SPOT_LIGHTS];
    mat4 cascadeMatrices[NUM_CASCADES];
    vec4 cascadeSplits;
    int pointLightCount;
    int spotLightCount;
    int hasDirectionalLight;
    float lightPadding;
};

#endif