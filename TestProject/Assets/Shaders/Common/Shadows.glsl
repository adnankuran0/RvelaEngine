#ifndef SHADOWS_GLSL
#define SHADOWS_GLSL

const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
);

float calculateDirectionalShadow(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, float shadowBias, float blurRadius) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || any(lessThan(projCoords.xy, vec2(0.0))) || any(greaterThan(projCoords.xy, vec2(1.0))))
        return 0.0;

    float bias = min(shadowBias * (1.0 - dot(normal, lightDir)), shadowBias);
    float currentDepth = projCoords.z - bias;

    if (blurRadius <= 0.0) {
        return step(texture(shadowMap, projCoords.xy).r, currentDepth);
    }

    vec2 texelSize = blurRadius / textureSize(shadowMap, 0);
    float shadow = 0.0;
    const int samples = 2;
    const int range = samples * 2 + 1;
    const float weight = 1.0 / float(range * range);
    
    for (int x = -samples; x <= samples; ++x) {
        for (int y = -samples; y <= samples; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += step(pcfDepth, currentDepth) * weight;
        }
    }
    return shadow;
}

float calculatePointLightShadow(samplerCubeArray pointShadowMap, int index, vec3 fragPos, vec3 lightPos, float farPlane, vec3 normal, float shadowBias, float blurRadius) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);

    if (currentDepth > farPlane) return 0.0;

    vec3 L = normalize(lightPos - fragPos);
    float bias = max(shadowBias * (1.0 - dot(normal, L)), shadowBias * 0.05); 
    currentDepth -= bias;

    if (blurRadius <= 0.0) {
        float closestDepth = texture(pointShadowMap, vec4(fragToLight, index)).r * farPlane;
        return step(closestDepth, currentDepth);
    }

    float diskRadius = blurRadius * (1.0 + (currentDepth / farPlane));
    float shadow = 0.0;
    const int samples = 8;
    
    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = fragToLight + vec3(poissonDisk[i] * diskRadius, 0.0);
        float closestDepth = texture(pointShadowMap, vec4(sampleDir, index)).r * farPlane;
        shadow += step(closestDepth, currentDepth);
    }
    return shadow / float(samples);
}

#endif