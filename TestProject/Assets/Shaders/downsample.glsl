#shader vertex
#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

#include "Common/FullscreenQuad.glsl"

void main()
{
    OutputFullscreenQuad(aPos, aTexCoords, TexCoords);
}

#shader fragment
#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

#include "Common/Sampling.glsl"

layout(binding = 0) uniform sampler2D u_Texture;
uniform vec2 u_TexelSize;

void main()
{
    vec3 color = BoxSample4Tap(u_Texture, TexCoords, u_TexelSize);
    FragColor = vec4(color, 1.0);
}