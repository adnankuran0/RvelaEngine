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

layout(binding = 0) uniform sampler2D u_SSGITexture;
layout(binding = 1) uniform sampler2D gDepth;
layout(binding = 2) uniform sampler2D gNormal;

uniform vec2 u_TexelSize;

void main()
{
    float centerDepth = texture(gDepth, TexCoords).r;
    if (centerDepth >= 0.999999)
    {
        discard;
    }

    vec3 centerNormal = normalize(texture(gNormal, TexCoords).xyz);
    float centerLinearDepth = -ReconstructViewPos(TexCoords, centerDepth, invProjection).z;

    vec3 result = vec3(0.0);
    float totalWeight = 0.0;

    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * u_TexelSize;
            vec2 sampleUV = TexCoords + offset;

            float sampleDepth = texture(gDepth, sampleUV).r;
            float sampleLinearDepth = -ReconstructViewPos(sampleUV, sampleDepth, invProjection).z;
            vec3 sampleNormal = normalize(texture(gNormal, sampleUV).xyz);

            float depthDiff = abs(centerLinearDepth - sampleLinearDepth);
            float depthWeight = exp(-depthDiff * 4.0);

            float normalWeight = max(0.0, dot(centerNormal, sampleNormal));
            normalWeight = pow(normalWeight, 16.0);

            float spatialWeight = exp(-float(x * x + y * y) / (2.0 * 2.0));

            float weight = depthWeight * normalWeight * spatialWeight;

            result += texture(u_SSGITexture, sampleUV).rgb * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight > 0.0001)
        result /= totalWeight;
    else
        result = texture(u_SSGITexture, TexCoords).rgb;

    if (dot(result, vec3(0.3333)) <= 0.0001)
    {
        discard;
    }

    FragColor = vec4(result, 1.0);
}
