#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 3) in vec2 aTexCoords;

#include "Common/Camera.glsl"
#include "Common/Lights.glsl"
#include "Common/Billboard.glsl"

uniform mat4 model;
uniform vec2 UVScale;
uniform vec2 UVOffset;
uniform int billboardMode;

out vec2 TexCoords;

void main() 
{
    TexCoords = aTexCoords * UVScale + UVOffset;

    BillboardTransform bb = CalculateBillboard(billboardMode, model, view, aPos, camPos);
    gl_Position = lightSpaceMatrix * bb.worldPos;
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