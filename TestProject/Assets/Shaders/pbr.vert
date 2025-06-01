#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec2 UVScale;
uniform vec2 UVOffset;

uniform mat4 lightSpaceMatrix;
uniform mat3 normalMatrix; 

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    Normal = normalMatrix * aNormal;

    gl_Position = projection * view * worldPos;

    TexCoords = aTexCoords * UVScale + UVOffset;

    FragPosLightSpace = lightSpaceMatrix * worldPos;
}
