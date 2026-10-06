#shader vertex
#version 460 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment
#version 460 core
out float FragColor;
in vec2 TexCoords;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"

layout(binding = 0) uniform sampler2D gNormal;
layout(binding = 1) uniform sampler2D gDepth;
layout(binding = 2) uniform sampler2D texNoise;

uniform float radius = 1.0;      
uniform float bias = 0.025;
uniform float intensity = 1.0;

void main()
{
    float depth = texture(gDepth, TexCoords).r;
    if (depth >= 0.999999)
    {
        FragColor = 1.0;
        return;
    }

    vec3 fragPos = ReconstructViewPos(TexCoords, depth, invProjection);
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);

    // horizon search
    vec2 noiseScale = windowSize * 0.25;
    vec2 randomVec = texture(texNoise, TexCoords * noiseScale).xy;
    float rotation = atan(randomVec.y, randomVec.x);
    float pixelRadius = max(radius * projection[1][1] * windowSize.y * 0.5 / max(-fragPos.z, 0.001), 1.0);

    const int DIRECTION_COUNT = 8;
    const int STEP_COUNT = 6;
    float occlusion = 0.0;
    for (int directionIndex = 0; directionIndex < DIRECTION_COUNT; ++directionIndex)
    {
        float angle = rotation + (float(directionIndex) + 0.5) * (6.2831853 / float(DIRECTION_COUNT));
        vec2 direction = vec2(cos(angle), sin(angle));
        float horizon = 0.0;

        for (int stepIndex = 1; stepIndex <= STEP_COUNT; ++stepIndex)
        {
            float stepFraction = float(stepIndex) / float(STEP_COUNT);
            float samplePixels = pixelRadius * stepFraction * stepFraction;
            vec2 sampleUV = TexCoords + direction * samplePixels / windowSize;
            if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
                continue;

            float sampleDepthValue = texture(gDepth, sampleUV).r;
            if (sampleDepthValue >= 0.999999)
                continue;

            vec3 samplePos = ReconstructViewPos(sampleUV, sampleDepthValue, invProjection);
            vec3 delta = samplePos - fragPos;
            float distanceToSample = length(delta);
            if (distanceToSample <= bias || distanceToSample >= radius)
                continue;

            float normalFacing = dot(normal, delta / distanceToSample);
            float distanceFade = 1.0 - smoothstep(radius * 0.5, radius, distanceToSample);
            horizon = max(horizon, max(normalFacing - bias, 0.0) * distanceFade);
        }

        occlusion += horizon;
    }

    float visibility = clamp(1.0 - occlusion / float(DIRECTION_COUNT), 0.0, 1.0);
    FragColor = pow(visibility, intensity);
}
