#version 460 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

layout(location = 0) out vec3 gNormal;
layout(location = 1) out float gRoughness;
layout(location = 2) out float gMetallic;

uniform bool useRoughnessMap;
uniform bool useMetallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicMap;
uniform float roughness;
uniform float metallic;

void main()
{
    gNormal = normalize(Normal);
    gRoughness = useRoughnessMap ? texture(roughnessMap, TexCoords).r : roughness;
    gMetallic = useMetallicMap ? texture(metallicMap, TexCoords).r : metallic;
}