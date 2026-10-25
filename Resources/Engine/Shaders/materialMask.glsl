#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 3) in vec2 aTexCoords;

#ifdef SKELETAL
layout(location = 4) in uvec4 aBoneIDs;
layout(location = 5) in vec4 aWeights;

const int MAX_BONES = 200;
uniform mat4 u_BoneMatrices[MAX_BONES];
#else
#include "Common/Billboard.glsl"
uniform int billboardMode;
#endif

#include "Common/Camera.glsl"

uniform mat4 model;
uniform vec2 UVScale;
uniform vec2 UVOffset;
uniform float u_OutlineWidth;
uniform bool u_OutlineFixedSize;

out vec2 TexCoords;
out float v_Width;

void main()
{
    TexCoords = aTexCoords * UVScale + UVOffset;

#ifdef SKELETAL
    mat4 skinMatrix = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        if (aWeights[i] <= 0.0) continue;
        if (aBoneIDs[i] < MAX_BONES)
        {
            skinMatrix += u_BoneMatrices[aBoneIDs[i]] * aWeights[i];
            totalWeight += aWeights[i];
        }
    }

    if (totalWeight <= 0.0001)
        skinMatrix = mat4(1.0);

    vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
    vec4 worldPos = model * skinnedPos;
    vec4 clipPos = projection * view * worldPos;
#else
    BillboardTransform bb = CalculateBillboard(billboardMode, model, view, aPos, camPos);
    vec4 clipPos = projection * view * bb.worldPos;
#endif

    gl_Position = clipPos;

    if (u_OutlineFixedSize)
    {
        v_Width = u_OutlineWidth;
    }
    else
    {
        float dist = max(clipPos.w, 0.5);
        v_Width = clamp(u_OutlineWidth * (5.0 / dist), 0.5, 12.0);
    }
}

#shader fragment
#version 460 core

in vec2 TexCoords;
in float v_Width;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_Info;

uniform vec4 u_OutlineColor;
uniform bool u_OutlineXRay;
layout(binding = 0) uniform sampler2D albedoMap;
uniform bool useAlbedoMap;
uniform vec4 albedoColor;
uniform int transparencyMode;
uniform float alphaCutoff;

void main()
{
    if (transparencyMode == 2 || transparencyMode == 3)
    {
        float alpha = (useAlbedoMap ? texture(albedoMap, TexCoords).a : 1.0) * albedoColor.a;
        if (alpha < alphaCutoff)
        {
            discard;
        }
    }

    o_Color = u_OutlineColor;
    o_Info = vec4(v_Width, u_OutlineXRay ? 1.0 : 0.0, gl_FragCoord.z, 1.0);
}
