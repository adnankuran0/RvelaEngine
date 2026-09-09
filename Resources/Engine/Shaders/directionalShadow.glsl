#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 3) in vec2 aTexCoords;

#ifdef SKELETAL
layout(location = 4) in uvec4 aBoneIDs;
layout(location = 5) in vec4 aWeights;

const int MAX_BONES = 100;
uniform mat4 u_BoneMatrices[MAX_BONES];
#else
#include "Common/Billboard.glsl"
uniform int billboardMode;
#endif

#include "Common/Camera.glsl"
#include "Common/Lights.glsl"

uniform mat4 model;
uniform vec2 UVScale;
uniform vec2 UVOffset;

out vec2 TexCoords;

void main() 
{
    TexCoords = aTexCoords * UVScale + UVOffset;

#ifdef SKELETAL
    mat4 skinMatrix = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        if (aWeights[i] <= 0.0) continue;
        skinMatrix += u_BoneMatrices[aBoneIDs[i]] * aWeights[i];
        totalWeight += aWeights[i];
    }

    if (totalWeight <= 0.0001)
        skinMatrix = mat4(1.0);

    vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
    vec4 worldPos = model * skinnedPos;
    gl_Position = lightSpaceMatrix * worldPos;
#else
    BillboardTransform bb = CalculateBillboard(billboardMode, model, view, aPos, camPos);
    gl_Position = lightSpaceMatrix * bb.worldPos;
#endif
}

#shader fragment
#version 460 core
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D albedoMap;
uniform bool useAlbedoMap;
uniform vec4 albedoColor;
uniform int transparencyMode;
uniform float alphaCutoff;

void main() 
{
    if (transparencyMode == 2) 
    {
        float alpha = (useAlbedoMap ? texture(albedoMap, TexCoords).a : 1.0) * albedoColor.a;
        if (alpha < alphaCutoff)
            discard;
    }
}