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

layout(location = 0) out float FragColor;
in vec2 TexCoords;

#include "Common/Camera.glsl"
#include "Common/Depth.glsl"

layout(binding = 0) uniform sampler2D u_RawSSAO;
layout(binding = 1) uniform sampler2D gDepth;
layout(binding = 2) uniform sampler2D gNormal;

uniform vec2 u_TexelSize;

void main()
{
    float centerDepth = texture(gDepth, TexCoords).r;
    if (centerDepth >= 0.999999)
    {
        FragColor = 1.0;
        return;
    }

    float centerAO = texture(u_RawSSAO, TexCoords).r;
    vec3 centerPos = ReconstructViewPos(TexCoords, centerDepth, invProjection);

    vec3 dX = dFdx(centerPos);
    vec3 dY = dFdy(centerPos);
    vec3 centerNormal = cross(dX, dY);
    if (dot(centerNormal, centerNormal) > 1e-8)
    {
        centerNormal = normalize(centerNormal);
        if (dot(centerNormal, -centerPos) < 0.0)
            centerNormal = -centerNormal;
    }
    else
    {
        centerNormal = normalize(texture(gNormal, TexCoords).xyz);
    }

    float totalAO = centerAO * 1.5;
    float totalWeight = 1.5;

    vec2 blurStep = u_TexelSize * 1.5;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            if (x == 0 && y == 0) continue;

            vec2 offsetUV = TexCoords + vec2(float(x), float(y)) * blurStep;
            if (offsetUV.x < 0.0 || offsetUV.x > 1.0 || offsetUV.y < 0.0 || offsetUV.y > 1.0)
                continue;

            float sampleDepth = texture(gDepth, offsetUV).r;
            if (sampleDepth >= 0.999999)
                continue;

            vec3 samplePos = ReconstructViewPos(offsetUV, sampleDepth, invProjection);

            float spatialDistSq = float(x * x + y * y);
            float spatialWeight = exp(-0.5 * spatialDistSq);

            float depthDiff = abs(centerPos.z - samplePos.z);
            float depthTolerance = max(0.10, abs(centerPos.z) * 0.05);
            float depthWeight = exp(-depthDiff / depthTolerance);

            vec3 sDX = dFdx(samplePos);
            vec3 sDY = dFdy(samplePos);
            vec3 sampleNormal = cross(sDX, sDY);
            if (dot(sampleNormal, sampleNormal) > 1e-8)
            {
                sampleNormal = normalize(sampleNormal);
                if (dot(sampleNormal, -samplePos) < 0.0)
                    sampleNormal = -sampleNormal;
            }
            else
            {
                sampleNormal = normalize(texture(gNormal, offsetUV).xyz);
            }

            float normalDot = max(0.0, dot(centerNormal, sampleNormal));
            float normalWeight = normalDot * normalDot;

            float weight = spatialWeight * depthWeight * normalWeight;
            float sampleAO = texture(u_RawSSAO, offsetUV).r;

            totalAO += sampleAO * weight;
            totalWeight += weight;
        }
    }

    FragColor = (totalWeight > 0.0001) ? (totalAO / totalWeight) : centerAO;
}
