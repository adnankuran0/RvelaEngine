#shader vertex
#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

#shader fragment
#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D uScreenTexture;

uniform float u_Radius = 3.0;
uniform float u_Intensity = 1.0;
uniform int u_Steps = 16;
uniform float u_Thickness = 0.5;

float InterleavedGradientNoise(vec2 screenPos)
{
    return fract(52.9829189 * fract(0.06711056 * screenPos.x + 0.00583715 * screenPos.y));
}

void CreateOrthonormalBasis(vec3 n, out vec3 t, out vec3 b)
{
    if (abs(n.z) < 0.999)
        t = normalize(cross(n, vec3(0.0, 0.0, 1.0)));
    else
        t = normalize(cross(n, vec3(1.0, 0.0, 0.0)));
    b = cross(n, t);
}

vec3 CosineSampleHemisphere(float u1, float u2)
{
    float r = sqrt(u1);
    float phi = 6.28318530718 * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - u1));
    return vec3(x, y, z);
}

void main()
{
    vec2 uv = TexCoords;
    float depth = texture(gDepth, uv).r;

    if (depth >= 0.999999)
    {
        FragColor = vec4(0.0);
        return;
    }

    vec3 viewPos = ReconstructViewPos(uv, depth, invProjection);
    vec3 viewNormal = normalize(texture(gNormal, uv).xyz);

    vec3 tangent, bitangent;
    CreateOrthonormalBasis(viewNormal, tangent, bitangent);

    float ign = InterleavedGradientNoise(gl_FragCoord.xy);

    vec3 accumulatedIndirect = vec3(0.0);
    float totalWeight = 0.0;

    const int NUM_RAYS = 4;
    int stepsPerRay = max(2, u_Steps / NUM_RAYS);

    float stepSize = u_Radius / float(stepsPerRay);

    for (int r = 0; r < NUM_RAYS; ++r)
    {
        float u1 = fract(float(r) * 0.6180339887 + ign);
        float u2 = fract(float(r) * 0.7548776662 + ign * 1.6180339887);

        vec3 localDir = CosineSampleHemisphere(u1, u2);
        vec3 rayDir = normalize(tangent * localDir.x + bitangent * localDir.y + viewNormal * localDir.z);

        float jitter = fract(ign + float(r) * 0.25);
        vec3 currentPos = viewPos + viewNormal * 0.03 + rayDir * (stepSize * (jitter * 0.8 + 0.2));

        for (int s = 0; s < stepsPerRay; ++s)
        {
            vec4 clip = projection * vec4(currentPos, 1.0);
            if (clip.w <= 0.0) break;

            vec2 sampleUV = (clip.xy / clip.w) * 0.5 + 0.5;

            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
                break;

            float sampleDepth = texture(gDepth, sampleUV).r;
            if (sampleDepth >= 0.999999)
            {
                currentPos += rayDir * stepSize;
                continue;
            }

            vec3 sampleViewPos = ReconstructViewPos(sampleUV, sampleDepth, invProjection);

            float rayDepth = -currentPos.z;
            float sceneDepth = -sampleViewPos.z;
            float depthDiff = rayDepth - sceneDepth;

            if (depthDiff > 0.01 && depthDiff < u_Thickness)
            {
                vec3 hitNormal = normalize(texture(gNormal, sampleUV).xyz);
                if (dot(hitNormal, -rayDir) > 0.05)
                {
                    vec2 edge = min(sampleUV, 1.0 - sampleUV);
                    float edgeFactor = min(edge.x, edge.y);
                    float edgeFade = smoothstep(0.0, 0.08, edgeFactor);

                    float hitDist = length(sampleViewPos - viewPos);
                    float distAtten = clamp(1.0 - (hitDist / u_Radius), 0.0, 1.0);
                    distAtten *= distAtten; // Quadratic falloff

                    float nDotL = max(0.0, dot(viewNormal, rayDir));

                    vec3 hitColor = texture(uScreenTexture, sampleUV).rgb;

                    hitColor = min(hitColor, vec3(10.0));

                    float weight = nDotL * edgeFade * distAtten;
                    accumulatedIndirect += hitColor * weight;
                    totalWeight += 1.0;
                    break;
                }
            }

            currentPos += rayDir * stepSize;
        }
    }

    if (totalWeight > 0.001)
    {
        accumulatedIndirect = (accumulatedIndirect / float(NUM_RAYS)) * u_Intensity;
    }
    else
    {
        accumulatedIndirect = vec3(0.0);
    }

    FragColor = vec4(accumulatedIndirect, 1.0);
}
