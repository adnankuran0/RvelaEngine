#shader vertex
#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment
#version 460 core

layout(location = 0) out float FragColor;
in vec2 TexCoords;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;

uniform vec3 u_LightDir; 
uniform float u_Distance;
uniform float u_Thickness;
uniform float u_Bias;
uniform float u_Intensity;
uniform int u_Steps;

float InterleavedGradientNoise(vec2 screenPos)
{
    return fract(52.9829189 * fract(0.06711056 * screenPos.x + 0.00583715 * screenPos.y));
}

void main()
{
    float depth = texture(gDepth, TexCoords).r;
    if (depth >= 0.999999)
    {
        FragColor = 0.0;
        return;
    }

    vec3 viewPos = ReconstructViewPos(TexCoords, depth, invProjection);
    vec3 viewNormal = texture(gNormal, TexCoords).xyz;
    if (dot(viewNormal, viewNormal) < 0.001)
    {
        FragColor = 0.0;
        return;
    }
    viewNormal = normalize(viewNormal);

    vec3 L = normalize(u_LightDir);
    float NdotL = dot(viewNormal, L);
    if (NdotL <= 0.0)
    {
        FragColor = 0.0;
        return;
    }
    float angleFactor = clamp(NdotL * 5.0, 0.0, 1.0);

    int steps = max(u_Steps, 4);
    float stepSize = u_Distance / float(steps);

    float ign = InterleavedGradientNoise(gl_FragCoord.xy);

    vec3 rayStart = viewPos + viewNormal * u_Bias + L * (u_Bias * 0.5);
    vec3 rayPos = rayStart + L * (stepSize * ign);

    float occlusion = 0.0;

    for (int i = 0; i < steps; ++i)
    {
        rayPos += L * stepSize;

        vec4 clipPos = projection * vec4(rayPos, 1.0);
        if (clipPos.w <= 0.0)
            break;

        vec2 sampleUV = (clipPos.xy / clipPos.w) * 0.5 + 0.5;
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            break;

        float sampleDepth = texture(gDepth, sampleUV).r;
        if (sampleDepth >= 0.999999)
            continue;

        vec3 scenePos = ReconstructViewPos(sampleUV, sampleDepth, invProjection);

        float rayDepth = -rayPos.z;
        float sceneDepth = -scenePos.z;
        float depthDiff = rayDepth - sceneDepth;

        if (depthDiff > 0.0 && depthDiff < u_Thickness)
        {
            float progress = (float(i) + ign) / float(steps);
            float distFade = 1.0 - progress;
            float thickFade = 1.0 - (depthDiff / u_Thickness);

            vec2 edgeDist = min(sampleUV, 1.0 - sampleUV);
            float edgeFade = clamp(min(edgeDist.x, edgeDist.y) * 20.0, 0.0, 1.0);

            float hitFactor = distFade * thickFade * edgeFade;
            occlusion = max(occlusion, hitFactor);

            if (occlusion >= 1.0)
                break;
        }
    }

    FragColor = clamp(occlusion * u_Intensity * angleFactor, 0.0, 1.0);
}
