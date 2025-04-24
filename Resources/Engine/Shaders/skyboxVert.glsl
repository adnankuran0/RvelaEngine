#version 330 core

layout (location = 0) in vec3 aPosition;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPosition;
    gl_Position = projection * view * vec4(aPosition, 1.0);
}
