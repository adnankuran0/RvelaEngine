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

float InterleavedGradientNoise(vec2 screenPos)
{
    return fract(52.9829189 * fract(0.06711056 * screenPos.x + 0.00583715 * screenPos.y));
}

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

    float pixelRadius = clamp(radius * projection[1][1] * windowSize.y * 0.25 / max(-fragPos.z, 0.1), 2.0, windowSize.y * 0.25);

    float ign = InterleavedGradientNoise(gl_FragCoord.xy);
    float baseAngle = ign * 6.2831853;

    const int DIRECTION_COUNT = 8;
    const int STEP_COUNT = 4;
    float occlusion = 0.0;
    float totalWeight = 0.0;

    for (int dirIdx = 0; dirIdx < DIRECTION_COUNT; ++dirIdx)
    {
        float angle = baseAngle + float(dirIdx) * (6.2831853 / float(DIRECTION_COUNT));
        vec2 dir = vec2(cos(angle), sin(angle));

        for (int stepIdx = 1; stepIdx <= STEP_COUNT; ++stepIdx)
        {
            float stepFraction = (float(stepIdx) + ign) / (float(STEP_COUNT) + 1.0);
            float stepPixels = pixelRadius * stepFraction;
            vec2 sampleUV = TexCoords + dir * (stepPixels / windowSize);

            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
                continue;

            float sampleDepth = texture(gDepth, sampleUV).r;
            if (sampleDepth >= 0.999999)
                continue;

            vec3 samplePos = ReconstructViewPos(sampleUV, sampleDepth, invProjection);
            vec3 delta = samplePos - fragPos;
            float dist = length(delta);

            if (dist > radius || dist < 0.01)
                continue;

            float nDotD = dot(normal, delta / dist);

            if (nDotD > bias)
            {
                float distFade = 1.0 - (dist / radius);
                occlusion += (nDotD - bias) * distFade * distFade;
            }
            totalWeight += 1.0;
        }
    }

    float ao = 1.0;
    if (totalWeight > 0.0)
    {
        ao = clamp(1.0 - (occlusion / totalWeight) * intensity * 2.0, 0.0, 1.0);
    }

    FragColor = ao;
}
