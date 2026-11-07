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
layout(location = 0) out vec4 FragColor;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"

layout(binding = 0) uniform sampler2D u_FogTexture;
layout(binding = 1) uniform sampler2D gDepth;

uniform vec2 u_TexelSize;
uniform bool u_ApplyBilateral;

void main()
{
    if (!u_ApplyBilateral)
    {
        FragColor = texture(u_FogTexture, TexCoords);
        return;
    }

    float centerDepth = texture(gDepth, TexCoords).r;
    float centerLinearDepth = -ReconstructViewPos(TexCoords, centerDepth, invProjection).z;

    vec4 totalColor = vec4(0.0);
    float totalWeight = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 offsetUV = TexCoords + vec2(float(x), float(y)) * u_TexelSize * 2.0;
            float sampleDepth = texture(gDepth, offsetUV).r;
            float sampleLinearDepth = -ReconstructViewPos(offsetUV, sampleDepth, invProjection).z;

            float depthDiff = abs(centerLinearDepth - sampleLinearDepth);
            float depthWeight = exp(-depthDiff * 0.5);
            float spatialWeight = (x == 0 && y == 0) ? 1.0 : ((x == 0 || y == 0) ? 0.5 : 0.25);

            float weight = depthWeight * spatialWeight;

            vec4 fogSample = texture(u_FogTexture, offsetUV);
            totalColor += fogSample * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight > 0.0001)
        FragColor = totalColor / totalWeight;
    else
        FragColor = texture(u_FogTexture, TexCoords);
}
