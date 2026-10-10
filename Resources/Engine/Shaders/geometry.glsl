#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;

#ifdef SKELETAL
layout(location = 3) in vec2 aTexCoords;
layout(location = 4) in uvec4 aBoneIDs;
layout(location = 5) in vec4 aWeights;

const int MAX_BONES = 100;
uniform mat4 u_BoneMatrices[MAX_BONES];
#else
layout(location = 3) in vec3 aBitangent;
layout(location = 4) in vec2 aTexCoords;

#include "Common/Billboard.glsl"
uniform int billboardMode;
#endif

#include "Common/Camera.glsl"

uniform mat4 model;
uniform mat3 normalMatrix;
uniform vec2 UVScale;
uniform vec2 UVOffset;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 WorldPos;
out vec3 WorldNormal;
out vec3 LocalPos;
out vec3 LocalNormal;

void main()
{
    vec4 worldPos;
    vec3 calculatedViewNormal;

#ifdef SKELETAL
    mat4 skinMatrix = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        if (aWeights[i] <= 0.0) continue;
        skinMatrix += u_BoneMatrices[aBoneIDs[i]] * aWeights[i];
        totalWeight += aWeights[i];
    }

    if (totalWeight <= 0.0001)
        skinMatrix = mat4(1.0);

    vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
    vec3 skinnedNormal = mat3(skinMatrix) * aNormal;

    worldPos = model * skinnedPos;
    calculatedViewNormal = mat3(view) * (normalMatrix * skinnedNormal);
#else
    if (billboardMode == 0)
    {
        worldPos = model * vec4(aPos, 1.0);
        calculatedViewNormal = mat3(view) * (normalMatrix * aNormal);
    }
    else
    {
        BillboardTransform bb = CalculateBillboard(billboardMode, model, view, aPos, camPos);
        worldPos = bb.worldPos;

        if (billboardMode == 1)
        {
            calculatedViewNormal = vec3(0.0, 0.0, 1.0);
        }
        else
        {
            calculatedViewNormal = mat3(view) * bb.normal;
        }
    }
#endif

    FragPos = vec3(view * worldPos);
    Normal = calculatedViewNormal;
    WorldPos = worldPos.xyz;
#ifdef SKELETAL
    WorldNormal = normalize(normalMatrix * skinnedNormal);
    LocalPos = skinnedPos.xyz;
    LocalNormal = normalize(skinnedNormal);
#else
    WorldNormal = normalize(normalMatrix * aNormal);
    LocalPos = aPos;
    LocalNormal = normalize(aNormal);
#endif
    TexCoords = aTexCoords * UVScale + UVOffset;
    gl_Position = projection * view * worldPos;
}

#shader fragment
#version 460 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 WorldNormal;
in vec3 LocalPos;
in vec3 LocalNormal;

layout(location = 0) out vec3 gNormal;
layout(location = 1) out float gRoughness;
layout(location = 2) out float gMetallic;

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D roughnessMap;
layout(binding = 2) uniform sampler2D metallicMap;

uniform bool useAlbedoMap;
uniform bool useRoughnessMap;
uniform bool useMetallicMap;

uniform bool useTriplanar;
uniform bool useWorldTriplanar;
uniform float triplanarSharpness;
uniform vec2 UVScale;
uniform vec2 UVOffset;

uniform vec4 albedoColor;
uniform float roughness;
uniform float metallic;

uniform int transparencyMode;
uniform float alphaCutoff;

vec3 getTriplanarWeights(vec3 normal, float sharpness)
{
    vec3 w = pow(abs(normal), vec3(sharpness));
    return w / max(0.00001, (w.x + w.y + w.z));
}

vec4 sampleTriplanar(sampler2D tex, vec3 p, vec3 weights)
{
    vec4 sampX = texture(tex, p.zy * vec2(-1.0, 1.0));
    vec4 sampY = texture(tex, p.xz);
    vec4 sampZ = texture(tex, p.xy);
    return sampX * weights.x + sampY * weights.y + sampZ * weights.z;
}

void main()
{
    if (useTriplanar)
    {
        vec3 triPos = useWorldTriplanar ? WorldPos : LocalPos;
        vec3 triGeomNormal = useWorldTriplanar ? WorldNormal : normalize(LocalNormal);
        vec3 triP = vec3(triPos.x * UVScale.x, triPos.y * UVScale.y, triPos.z * UVScale.x) + vec3(UVOffset.x, UVOffset.y, 0.0);
        vec3 triWeights = getTriplanarWeights(triGeomNormal, triplanarSharpness);

        if (transparencyMode == 2)
        {
            float alpha = (useAlbedoMap ? sampleTriplanar(albedoMap, triP, triWeights).a : 1.0) * albedoColor.a;
            if (alpha < alphaCutoff)
            {
                discard;
            }
        }

        gNormal = normalize(Normal);
        gRoughness = useRoughnessMap ? sampleTriplanar(roughnessMap, triP, triWeights).r : roughness;
        gMetallic = useMetallicMap ? sampleTriplanar(metallicMap, triP, triWeights).r : metallic;
    }
    else
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
}