#shader vertex
#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 0) out vec2 TexCoord;

void main()
{
    TexCoord = (aPos + 1.0) * 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

#shader fragment
#version 460 core

layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"

layout(binding = 0) uniform sampler2D uDepthTexture;
layout(binding = 1) uniform sampler2D uNormalTexture;
layout(binding = 2) uniform sampler2D uRoughnessTexture;
layout(binding = 3) uniform sampler2D uMetallicTexture;
layout(binding = 4) uniform sampler2D uScreenTexture;
layout(binding = 5) uniform samplerCube uSkybox;

uniform int uMaxSteps = 32;
uniform float uMaxDistance = 25.0;
uniform float uThickness = 0.5;
uniform float uEdgeFade = 0.15;
uniform float uMaxRoughness = 0.6;

void main()
{
    vec2 uv = TexCoord;

    float depth = texture(uDepthTexture, uv).r;
    if (depth >= 0.99999)
    {
        FragColor = vec4(0.0);
        return;
    }

    float roughness = texture(uRoughnessTexture, uv).r;
    if (roughness > uMaxRoughness)
    {
        FragColor = vec4(0.0);
        return;
    }

    float metallic = texture(uMetallicTexture, uv).r;

    vec3 viewPos = ReconstructViewPos(uv, depth, invProjection);
    vec3 viewNormal = normalize(texture(uNormalTexture, uv).xyz);

    vec3 viewDir = normalize(viewPos);
    vec3 reflectDir = normalize(reflect(viewDir, viewNormal));

    if (reflectDir.z > 0.05)
    {
        FragColor = vec4(0.0);
        return;
    }

    float stepSize = uMaxDistance / float(uMaxSteps);
    vec3 rayStep = reflectDir * stepSize;
    vec3 rayPos = viewPos + rayStep * 0.5;

    bool hit = false;
    vec2 hitUV = vec2(0.0);
    float hitDistance = 0.0;

    for (int i = 0; i < uMaxSteps; ++i)
    {
        rayPos += rayStep;

        vec4 clip = projection * vec4(rayPos, 1.0);
        if (clip.w <= 0.0) break;

        vec2 sampleUV = (clip.xy / clip.w) * 0.5 + 0.5;
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            break;

        float sampleDepth = texture(uDepthTexture, sampleUV).r;
        if (sampleDepth >= 0.99999) continue;

        vec3 scenePos = ReconstructViewPos(sampleUV, sampleDepth, invProjection);

        float rayDepth = -rayPos.z;
        float sceneDepth = -scenePos.z;
        float depthDiff = rayDepth - sceneDepth;

        if (depthDiff > 0.0 && depthDiff < uThickness)
        {
            vec3 startP = rayPos - rayStep;
            vec3 endP = rayPos;

            for (int j = 0; j < 4; ++j)
            {
                vec3 midP = (startP + endP) * 0.5;
                vec4 midClip = projection * vec4(midP, 1.0);
                vec2 midUV = (midClip.xy / midClip.w) * 0.5 + 0.5;
                float midDepth = texture(uDepthTexture, midUV).r;
                vec3 midScene = ReconstructViewPos(midUV, midDepth, invProjection);
                float diff = (-midP.z) - (-midScene.z);

                if (diff > 0.0 && diff < uThickness)
                {
                    endP = midP;
                    sampleUV = midUV;
                }
                else
                {
                    startP = midP;
                }
            }

            vec3 hitNormal = normalize(texture(uNormalTexture, sampleUV).xyz);
            if (dot(hitNormal, reflectDir) < 0.0)
            {
                hit = true;
                hitUV = sampleUV;
                hitDistance = length(endP - viewPos);
                break;
            }
        }
    }

    if (!hit)
    {
        FragColor = vec4(0.0);
        return;
    }

    vec2 edgeDist = min(hitUV, 1.0 - hitUV);
    float edgeFactor = min(edgeDist.x, edgeDist.y);
    float edgeFade = smoothstep(0.0, uEdgeFade, edgeFactor);

    float distFade = 1.0 - smoothstep(uMaxDistance * 0.5, uMaxDistance, hitDistance);

    float roughnessFade = 1.0 - smoothstep(0.0, uMaxRoughness, roughness);

    float dirFade = clamp(-reflectDir.z * 5.0, 0.0, 1.0);

    float nDotV = clamp(dot(viewNormal, -viewDir), 0.0, 1.0);
    float fresnel = mix(0.04, 1.0, pow(1.0 - nDotV, 5.0));
    float finalAlpha = mix(fresnel, 1.0, metallic) * edgeFade * distFade * roughnessFade * dirFade;

    if (finalAlpha <= 0.001)
    {
        FragColor = vec4(0.0);
        return;
    }

    vec3 hitColor = texture(uScreenTexture, hitUV).rgb;
    FragColor = vec4(hitColor, finalAlpha);
}