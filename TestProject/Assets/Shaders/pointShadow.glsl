#shader vertex
#version 460 core
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_AMD_vertex_shader_layer : enable
#extension GL_NV_viewport_array2 : enable

layout (location = 0) in vec3 aPos;
layout (location = 3) in vec2 aTexCoords;

#include "Common/Billboard.glsl"

uniform mat4 model;
uniform mat4 shadowMatrix;  
uniform int baseLayer;
uniform int currentFace;    
uniform vec2 UVScale;
uniform vec2 UVOffset;

uniform int billboardMode;
uniform mat4 cameraView;
uniform vec3 camPos;

out vec4 FragPos;
out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords * UVScale + UVOffset;

    BillboardTransform bb = CalculateBillboard(billboardMode, model, cameraView, aPos, camPos);

    FragPos = bb.worldPos;
    gl_Position = shadowMatrix * FragPos;
    gl_Layer = baseLayer + currentFace;
}

#shader fragment
#version 460 core 
in vec4 FragPos;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D albedoMap;
uniform bool useAlbedoMap;
uniform vec4 albedoColor;
uniform int transparencyMode;
uniform float alphaCutoff;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    if (transparencyMode == 2) {
        float alpha = (useAlbedoMap ? texture(albedoMap, TexCoords).a : 1.0) * albedoColor.a;
        if (alpha < alphaCutoff)
            discard;
    }

    float lightDistance = length(FragPos.xyz - lightPos);
    lightDistance = lightDistance / far_plane;
    gl_FragDepth = lightDistance;
}