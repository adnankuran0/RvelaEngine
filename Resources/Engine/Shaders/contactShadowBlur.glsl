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

layout(binding = 0) uniform sampler2D u_ContactShadowTexture;
layout(binding = 1) uniform sampler2D gDepth;
layout(binding = 2) uniform sampler2D gNormal;

uniform vec2 u_TexelSize;

void main()
{
    float centerDepth = texture(gDepth, TexCoords).r;
    if (centerDepth >= 0.999999)
    {
        FragColor = 0.0;
        return;
    }

    vec3 centerPos = ReconstructViewPos(TexCoords, centerDepth, invProjection);
    vec3 centerNormal = texture(gNormal, TexCoords).xyz;
    if (dot(centerNormal, centerNormal) < 0.001)
    {
        FragColor = 0.0;
        return;
    }
    centerNormal = normalize(centerNormal);

    float centerShadow = texture(u_ContactShadowTexture, TexCoords).r;

    float totalShadow = centerShadow;
    float totalWeight = 1.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            if (x == 0 && y == 0) continue;

            vec2 offsetUV = TexCoords + vec2(float(x), float(y)) * u_TexelSize;
            if (offsetUV.x < 0.0 || offsetUV.x > 1.0 || offsetUV.y < 0.0 || offsetUV.y > 1.0)
                continue;

            float neighborDepth = texture(gDepth, offsetUV).r;
            if (neighborDepth >= 0.999999)
                continue;

            vec3 neighborPos = ReconstructViewPos(offsetUV, neighborDepth, invProjection);
            vec3 neighborNormal = normalize(texture(gNormal, offsetUV).xyz);

            float depthDiff = abs(centerPos.z - neighborPos.z);
            float depthWeight = exp(-depthDiff * 15.0);
            float normalWeight = max(0.0, dot(centerNormal, neighborNormal));
            normalWeight = normalWeight * normalWeight;

            float weight = depthWeight * normalWeight;
            float neighborShadow = texture(u_ContactShadowTexture, offsetUV).r;

            totalShadow += neighborShadow * weight;
            totalWeight += weight;
        }
    }

    FragColor = totalShadow / totalWeight;
}
