#version 460 core
layout (location = 0) in vec2 aPos;

uniform mat4 invProjection;
uniform mat4 invView;

out vec3 worldDir;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vec4 viewPos = invProjection * vec4(aPos, 1.0, 1.0);
    viewPos /= viewPos.w;
    vec4 worldPos = invView * vec4(viewPos.xyz, 0.0);
    worldDir = normalize(worldPos.xyz);
}