#version 410 core

layout(location = 0) in vec2 aPos;
layout(location = 0) out vec2 TexCoord;

void main()
{
    TexCoord = (aPos + 1.0) * 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
