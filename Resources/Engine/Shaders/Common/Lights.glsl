#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL

#define MAX_POINT_LIGHTS 20
#define NUM_CASCADES 4

struct PointLight {
    vec4 position;       // xyz: pos, w: falloff
    vec4 colorIntensity; // rgb: color, w: intensity
    float radius;
    float shadowBias;
    float blurRadius;
    int shadowIndex;
};

struct DirectionalLight {
    vec4 direction;      // xyz: dir, w: castShadows (1.0/0.0)
    vec4 colorIntensity; // rgb: color, w: intensity
    float shadowBias;
    float blurRadius;
    float normalBias;
    int cascadeCount;
};

layout(std140, binding = 1) uniform LightData {
    DirectionalLight directionalLight;
    PointLight pointLights[MAX_POINT_LIGHTS];
    mat4 cascadeMatrices[NUM_CASCADES];
    vec4 cascadeSplits;
    int pointLightCount;
    int hasDirectionalLight;
    vec2 lightPadding;
};

#endif