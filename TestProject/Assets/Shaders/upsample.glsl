#shader vertex
#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment
#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_UpsampleTex;
uniform float u_Intensity = 0.5; 

void main()
{   
    FragColor = vec4(texture(u_UpsampleTex, TexCoords).rgb * u_Intensity, 1.0);
}