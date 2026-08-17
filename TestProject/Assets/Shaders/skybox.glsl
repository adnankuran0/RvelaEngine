#shader vertex
#version 460 core
layout(location = 0) in vec3 aPosition;
out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPosition; 
    vec4 pos = projection * view * vec4(aPosition, 1.0);
    gl_Position = pos.xyww;
}

#shader fragment
#version 460 core

in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;

void main()
{
    FragColor = vec4(texture(skybox, TexCoords).rgb, 1.0);
}