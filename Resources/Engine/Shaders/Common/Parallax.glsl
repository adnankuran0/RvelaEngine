#ifndef PARALLAX_GLSL
#define PARALLAX_GLSL

layout(binding = 5) uniform sampler2D heightMap;

uniform bool useHeightMap;
uniform float heightScale;
uniform bool invertHeight;

vec3 getNormalFromHeightMap(vec2 uv, vec2 ddx, vec2 ddy, float scale)
{
    vec2 texelSize = 1.0 / vec2(max(textureSize(heightMap, 0), ivec2(1)));
    float hL = textureGrad(heightMap, uv - vec2(texelSize.x, 0.0), ddx, ddy).r;
    float hR = textureGrad(heightMap, uv + vec2(texelSize.x, 0.0), ddx, ddy).r;
    float hD = textureGrad(heightMap, uv - vec2(0.0, texelSize.y), ddx, ddy).r;
    float hU = textureGrad(heightMap, uv + vec2(0.0, texelSize.y), ddx, ddy).r;

    if (invertHeight)
    {
        hL = 1.0 - hL;
        hR = 1.0 - hR;
        hD = 1.0 - hD;
        hU = 1.0 - hU;
    }

    float effScale = clamp(scale, 0.001, 10.0);
    float dHdX = (hR - hL) * effScale * 15.0;
    float dHdY = (hU - hD) * effScale * 15.0;

    return normalize(vec3(-dHdX, -dHdY, 1.0));
}

vec2 parallaxOcclusionMapping(vec2 texCoords, vec3 viewDirTS, vec2 ddx, vec2 ddy, out float outHitDepth)
{
    outHitDepth = 0.0;
    if (!useHeightMap || abs(heightScale) <= 0.00001)
        return texCoords;

    float vz = abs(viewDirTS.z);
    if (vz < 0.0001)
        return texCoords;

    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, clamp(vz, 0.0, 1.0));

    vec2 P = (viewDirTS.xy / max(vz, 0.05)) * heightScale;
    vec2 deltaTexCoords = P / numLayers;
    float layerDepth = 1.0 / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentLayerDepth = 0.0;

    float initialHeight = textureGrad(heightMap, currentTexCoords, ddx, ddy).r;
    if (invertHeight) initialHeight = 1.0 - initialHeight;
    float currentDepthMapValue = 1.0 - initialHeight;

    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentLayerDepth += layerDepth;

        float h = textureGrad(heightMap, currentTexCoords, ddx, ddy).r;
        if (invertHeight) h = 1.0 - h;
        currentDepthMapValue = 1.0 - h;

        if (currentLayerDepth >= 1.0)
            break;
    }

    if (currentLayerDepth <= 0.00001)
    {
        outHitDepth = 0.0;
        return texCoords;
    }

    vec2 minUV = currentTexCoords + deltaTexCoords;
    vec2 maxUV = currentTexCoords;
    float minDepth = currentLayerDepth - layerDepth;
    float maxDepth = currentLayerDepth;

    for (int i = 0; i < 3; ++i)
    {
        vec2 midUV = 0.5 * (minUV + maxUV);
        float midDepth = 0.5 * (minDepth + maxDepth);
        float h = textureGrad(heightMap, midUV, ddx, ddy).r;
        if (invertHeight) h = 1.0 - h;
        float d = 1.0 - h;

        if (midDepth < d)
        {
            minUV = midUV;
            minDepth = midDepth;
        }
        else
        {
            maxUV = midUV;
            maxDepth = midDepth;
        }
    }

    float h0 = textureGrad(heightMap, minUV, ddx, ddy).r;
    if (invertHeight) h0 = 1.0 - h0;
    float d0 = (1.0 - h0) - minDepth;

    float h1 = textureGrad(heightMap, maxUV, ddx, ddy).r;
    if (invertHeight) h1 = 1.0 - h1;
    float d1 = maxDepth - (1.0 - h1);

    float denom = d0 + d1;
    float weight = (denom > 1e-5) ? clamp(d0 / denom, 0.0, 1.0) : 0.5;

    vec2 finalUV = mix(minUV, maxUV, weight);
    outHitDepth = mix(minDepth, maxDepth, weight);

    return finalUV;
}

float calculateParallaxShadow(vec2 hitUV, float hitDepth, vec3 lightDirTS, vec2 ddx, vec2 ddy, float scale)
{
    float lz = abs(lightDirTS.z);
    if (hitDepth <= 0.03 || lz <= 0.001 || abs(scale) <= 0.0001)
        return 1.0;

    const int shadowSteps = 5;
    float stepDepth = hitDepth / float(shadowSteps);
    vec2 P_light = (lightDirTS.xy / max(lz, 0.08)) * scale;
    vec2 deltaUV = (P_light * hitDepth) / float(shadowSteps);

    float currentDepth = hitDepth;
    vec2 currentUV = hitUV;
    float maxShadow = 0.0;

    for (int i = 1; i <= shadowSteps; ++i)
    {
        currentDepth -= stepDepth;
        currentUV += deltaUV;

        float h = textureGrad(heightMap, currentUV, ddx, ddy).r;
        if (invertHeight) h = 1.0 - h;
        float sDepth = 1.0 - h;

        if (sDepth < currentDepth)
        {
            float diff = (currentDepth - sDepth) * (float(shadowSteps - i) / float(shadowSteps));
            maxShadow = max(maxShadow, diff);
        }
    }

    return clamp(1.0 - maxShadow * 2.5, 0.0, 1.0);
}

#endif // PARALLAX_GLSL
