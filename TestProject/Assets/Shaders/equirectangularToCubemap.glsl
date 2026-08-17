#shader vertex
#version 460 core
layout (location = 0) in vec3 aPos;

out vec3 WorldPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    WorldPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}

#shader fragment
#version 460 core
out vec4 FragColor;
in vec3 WorldPos;

#include "Common/SphericalMath.glsl"

uniform sampler2D equirectangularMap;

void main()
{
    vec3 dir = normalize(WorldPos);
    vec2 uv = SampleSphericalMap(dir);
    vec3 color = texture(equirectangularMap, uv).rgb;
    FragColor = vec4(color, 1.0);
}