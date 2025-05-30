#version 330 core
in vec3 FragPos;
in vec3 Normal;

layout(location = 0) out vec3 gNormal;

void main()
{
    gNormal = normalize(Normal);
}