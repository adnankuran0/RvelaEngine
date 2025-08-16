#shader vertex
#version 460 core

layout (location = 0) in vec3 aPosition;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPosition;
    gl_Position = projection * view * vec4(aPosition, 1.0);
}

#shader fragment
#version 460 core

in vec3 TexCoords;

out vec4 FragColor;

layout(binding = 3) uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, TexCoords);
}
