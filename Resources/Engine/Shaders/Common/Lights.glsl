#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL

#define MAX_POINT_LIGHTS 20

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
    vec2 padding;
};

layout(std140, binding = 1) uniform LightData {
    DirectionalLight directionalLight;
    PointLight pointLights[MAX_POINT_LIGHTS];
    mat4 lightSpaceMatrix;
    int pointLightCount;
    int hasDirectionalLight;
    vec2 lightPadding;
};

#endif