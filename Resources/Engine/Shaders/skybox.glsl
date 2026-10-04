#shader vertex
#version 460 core
layout(location = 0) in vec3 aPosition;
out vec3 TexCoords;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
    TexCoords = aPosition; 
    mat4 viewNoTranslation = mat4(mat3(u_View));
    vec4 pos = u_Projection * viewNoTranslation * vec4(aPosition, 1.0);
    gl_Position = pos.xyww;
}

#shader fragment
#version 460 core

in vec3 TexCoords;
out vec4 FragColor;

layout(binding = 0) uniform samplerCube skybox;

void main()
{
    FragColor = vec4(texture(skybox, TexCoords).rgb, 1.0);
}