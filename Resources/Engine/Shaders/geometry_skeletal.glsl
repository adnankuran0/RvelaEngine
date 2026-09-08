#shader vertex
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec2 aTexCoords;
layout(location = 4) in uvec4 aBoneIDs;
layout(location = 5) in vec4 aWeights;

#include "Common/Camera.glsl"

const int MAX_BONES = 100;

uniform mat4 model;
uniform mat3 normalMatrix;
uniform vec2 UVScale;
uniform vec2 UVOffset;
uniform mat4 u_BoneMatrices[MAX_BONES];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
    mat4 skinMatrix = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        if (aBoneIDs[i] < 0) continue;
        skinMatrix += u_BoneMatrices[aBoneIDs[i]] * aWeights[i];
        totalWeight += aWeights[i];
    }

    if (totalWeight <= 0.0001)
        skinMatrix = mat4(1.0);

    vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
    vec3 skinnedNormal = mat3(skinMatrix) * aNormal;

    vec4 worldPos = model * skinnedPos;

    FragPos = vec3(view * worldPos);
    Normal = mat3(view) * (normalMatrix * skinnedNormal);
    TexCoords = aTexCoords * UVScale + UVOffset;
    gl_Position = projection * view * worldPos;
}

#shader fragment
#version 460 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

layout(location = 0) out vec3 gNormal;
layout(location = 1) out float gRoughness;
layout(location = 2) out float gMetallic;

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D roughnessMap;
layout(binding = 2) uniform sampler2D metallicMap;

uniform bool useAlbedoMap;
uniform bool useRoughnessMap;
uniform bool useMetallicMap;

uniform vec4 albedoColor;
uniform float roughness;
uniform float metallic;

uniform int transparencyMode;
uniform float alphaCutoff;

void main()
{
    if (transparencyMode == 2)
    {
        float alpha = (useAlbedoMap ? texture(albedoMap, TexCoords).a : 1.0) * albedoColor.a;
        if (alpha < alphaCutoff)
        {
            discard;
        }
    }

    gNormal = normalize(Normal);
    gRoughness = useRoughnessMap ? texture(roughnessMap, TexCoords).r : roughness;
    gMetallic = useMetallicMap ? texture(metallicMap, TexCoords).r : metallic;
}