#ifndef SHADOWS_GLSL
#define SHADOWS_GLSL

#include "Lights.glsl"

const vec2 POISSON_DISK[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
);

float sampleCascadeShadow(sampler2DArray shadowMap, int cascadeIndex, vec3 worldPos, vec3 normal, vec3 lightDir, float shadowBias, float normalBias, float blurRadius)
{
    float cascadeTexelScale = 1.0 + float(cascadeIndex) * 1.5;
    vec3 normalOffset = normal * (normalBias * cascadeTexelScale);
    vec4 fragPosLightSpace = cascadeMatrices[cascadeIndex] * vec4(worldPos + normalOffset, 1.0);

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || any(lessThan(projCoords.xy, vec2(0.0))) || any(greaterThan(projCoords.xy, vec2(1.0))))
        return 0.0;

    float NdotL = max(dot(normal, lightDir), 0.0);
    float slopeBias = max(shadowBias * (1.0 - NdotL), shadowBias * 0.1) * (1.0 + float(cascadeIndex) * 0.5);
    float currentDepth = projCoords.z - slopeBias;

    if (blurRadius <= 0.0) {
        float d = texture(shadowMap, vec3(projCoords.xy, float(cascadeIndex))).r;
        return (currentDepth > d) ? 1.0 : 0.0;
    }

    vec2 texelSize = (blurRadius * (1.0 + float(cascadeIndex) * 0.25)) / vec2(textureSize(shadowMap, 0).xy);
    float shadow = 0.0;

    for (int i = 0; i < 16; ++i) {
        vec2 offset = POISSON_DISK[i] * texelSize;
        float pcfDepth = texture(shadowMap, vec3(projCoords.xy + offset, float(cascadeIndex))).r;
        shadow += (currentDepth > pcfDepth) ? 1.0 : 0.0;
    }

    return shadow / 16.0;
}

float calculateDirectionalShadow(sampler2DArray shadowMap, vec3 worldPos, vec3 normal, vec3 lightDir, float viewDepth, float shadowBias, float normalBias, float blurRadius)
{
    int cascadeIndex = NUM_CASCADES - 1;
    for (int i = 0; i < NUM_CASCADES - 1; ++i) {
        if (viewDepth < cascadeSplits[i]) {
            cascadeIndex = i;
            break;
        }
    }

    float shadow = sampleCascadeShadow(shadowMap, cascadeIndex, worldPos, normal, lightDir, shadowBias, normalBias, blurRadius);

    if (cascadeIndex < NUM_CASCADES - 1) {
        float splitDist = cascadeSplits[cascadeIndex];
        float prevDist = (cascadeIndex == 0) ? 0.0 : cascadeSplits[cascadeIndex - 1];
        float cascadeRange = splitDist - prevDist;
        float blendThreshold = splitDist - cascadeRange * 0.15;

        if (viewDepth > blendThreshold) {
            float blendFactor = clamp((viewDepth - blendThreshold) / max(0.0001, (splitDist - blendThreshold)), 0.0, 1.0);
            float nextShadow = sampleCascadeShadow(shadowMap, cascadeIndex + 1, worldPos, normal, lightDir, shadowBias, normalBias, blurRadius);
            shadow = mix(shadow, nextShadow, blendFactor);
        }
    }

    float maxDist = cascadeSplits[NUM_CASCADES - 1];
    float fadeStart = maxDist * 0.85;
    if (viewDepth > fadeStart) {
        float fade = clamp((maxDist - viewDepth) / max(0.0001, (maxDist - fadeStart)), 0.0, 1.0);
        shadow *= fade;
    }

    return shadow;
}

float calculatePointLightShadow(samplerCubeArray pointShadowMap, int index, vec3 fragPos, vec3 lightPos, float farPlane, vec3 normal, float shadowBias, float blurRadius)
{
    vec3 L = normalize(lightPos - fragPos);

    vec3 normalOffset = normal * (shadowBias * 0.05);
    vec3 biasedFragPos = fragPos + normalOffset;
    vec3 fragToLight = biasedFragPos - lightPos;
    float currentDepth = length(fragToLight);

    if (currentDepth > farPlane) return 0.0;

    float NdotL = max(dot(normal, L), 0.0);
    float bias = max(shadowBias * (1.0 - NdotL), shadowBias * 0.1);
    float shadowDepth = (currentDepth - bias) / farPlane;

    if (blurRadius <= 0.0) {
        float closestDepth = texture(pointShadowMap, vec4(fragToLight, float(index))).r;
        return (shadowDepth > closestDepth) ? 1.0 : 0.0;
    }

    float diskRadius = blurRadius * (1.0 + (currentDepth / farPlane));
    float shadow = 0.0;

    for (int i = 0; i < 16; ++i) {
        vec3 sampleDir = fragToLight + vec3(POISSON_DISK[i] * diskRadius, POISSON_DISK[15 - i].x * diskRadius);
        float closestDepth = texture(pointShadowMap, vec4(sampleDir, float(index))).r;
        shadow += (shadowDepth > closestDepth) ? 1.0 : 0.0;
    }

    return shadow / 16.0;
}

#endif