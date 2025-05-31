#version 460 core
precision highp float;

layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_UpsampleTex;
layout(binding = 1) uniform sampler2D u_BaseTex;

const float upsampleWeight = 0.7;
const float baseWeight = 0.3;

void main()
{
    vec3 upsampled = textureLod(u_UpsampleTex, TexCoords, 0.0).rgb;
    vec3 base = textureLod(u_BaseTex, TexCoords, 0.0).rgb;
    
    FragColor = vec4(mix(base, upsampled, upsampleWeight), 1.0);
}