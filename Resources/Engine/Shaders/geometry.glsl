#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;

layout(location = 3) in vec2 aTexCoords;

#ifdef SKELETAL
layout(location = 4) in uvec4 aBoneIDs;
layout(location = 5) in vec4 aWeights;

const int MAX_BONES = 200;
uniform mat4 u_BoneMatrices[MAX_BONES];
#else
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
out vec3 Tangent;
out vec3 Bitangent;
out vec3 LocalPos;
out vec3 LocalNormal;

void main()
{
    vec4 worldPos;
    vec3 calculatedViewNormal;
    vec3 N, T, B;

#ifdef SKELETAL
    mat4 skinMatrix = mat4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        if (aWeights[i] <= 0.0) continue;
        if (aBoneIDs[i] < MAX_BONES)
        {
            skinMatrix += u_BoneMatrices[aBoneIDs[i]] * aWeights[i];
            totalWeight += aWeights[i];
        }
    }

    if (totalWeight <= 0.0001)
        skinMatrix = mat4(1.0);

    vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
    mat3 skinMatrixIT = mat3(skinMatrix);
    vec3 skinnedNormal = skinMatrixIT * aNormal;
    vec3 skinnedTangent = skinMatrixIT * aTangent;

    worldPos = model * skinnedPos;
    calculatedViewNormal = mat3(view) * (normalMatrix * skinnedNormal);

    N = normalize(normalMatrix * skinnedNormal);
    T = normalize(mat3(model) * skinnedTangent);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));
#else
    if (billboardMode == 0)
    {
        worldPos = model * vec4(aPos, 1.0);
        calculatedViewNormal = mat3(view) * (normalMatrix * aNormal);
        N = normalize(normalMatrix * aNormal);
        T = normalize(mat3(model) * aTangent);
        T = normalize(T - dot(T, N) * N);
        B = normalize(cross(N, T));
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
        N = normalize(bb.normal);
        T = normalize(bb.tangent);
        B = normalize(bb.bitangent);
    }
#endif

    FragPos = vec3(view * worldPos);
    Normal = calculatedViewNormal;
    WorldPos = worldPos.xyz;
    WorldNormal = N;
    Tangent = T;
    Bitangent = B;
#ifdef SKELETAL
    LocalPos = skinnedPos.xyz;
    LocalNormal = normalize(skinnedNormal);
#else
    LocalPos = aPos;
    LocalNormal = normalize(aNormal);
#endif
    TexCoords = aTexCoords * UVScale + UVOffset;
    gl_Position = projection * view * worldPos;
}

#shader fragment
#version 460 core

#include "Common/Camera.glsl"

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 WorldNormal;
in vec3 Tangent;
in vec3 Bitangent;
in vec3 LocalPos;
in vec3 LocalNormal;

layout(location = 0) out vec3 gNormal;
layout(location = 1) out float gRoughness;
layout(location = 2) out float gMetallic;

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D roughnessMap;
layout(binding = 2) uniform sampler2D metallicMap;
layout(binding = 3) uniform sampler2D normalMap;

uniform bool useAlbedoMap;
uniform bool useRoughnessMap;
uniform bool useMetallicMap;
uniform bool useNormalMap;
uniform float normalScale;

uniform bool useTriplanar;
uniform bool useWorldTriplanar;
uniform float triplanarSharpness;
uniform vec2 UVScale;
uniform vec2 UVOffset;
uniform mat3 normalMatrix;

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

vec3 unpackNormalRG(sampler2D tex, vec2 uv)
{
    vec2 rg = texture(tex, uv).rg * 2.0 - 1.0;
    rg *= normalScale;
    float z = sqrt(max(0.0, 1.0 - dot(rg, rg)));
    return vec3(rg, z);   
}

vec3 sampleTriplanarNormal(sampler2D tex, vec3 p, vec3 weights, vec3 geomNormal)
{
    vec3 nX = unpackNormalRG(tex, p.zy * vec2(-1.0, 1.0));
    vec3 nY = unpackNormalRG(tex, p.xz);
    vec3 nZ = unpackNormalRG(tex, p.xy);

    vec3 s = vec3(geomNormal.x >= 0.0 ? 1.0 : -1.0,
                  geomNormal.y >= 0.0 ? 1.0 : -1.0,
                  geomNormal.z >= 0.0 ? 1.0 : -1.0);

    vec3 wX = vec3(nX.z * s.x, nX.y, -nX.x * s.x);
    vec3 wY = vec3(nY.x, nY.z * s.y, nY.y);
    vec3 wZ = vec3(nZ.x, nZ.y, nZ.z * s.z);

    return normalize(wX * weights.x + wY * weights.y + wZ * weights.z);
}

vec3 getNormalFromMap(vec2 texCoords, vec3 N, vec3 T, vec3 B) 
{
    if (!useNormalMap) return N;
    
    vec2 rg = texture(normalMap, texCoords).rg * 2.0 - 1.0;
    float b = sqrt(max(0.0, 1.0 - dot(rg, rg)));
    vec3 tangentNormal = vec3(rg, b);
    
    tangentNormal.xy *= normalScale;
    
    float tnLen = length(tangentNormal);
    if (tnLen < 0.001) return N;
    tangentNormal /= tnLen;
    
    return normalize(mat3(T, B, N) * tangentNormal);
}

void main()
{
    if (transparencyMode == 1)
        discard;

    vec3 T = normalize(Tangent);
    vec3 N = normalize(WorldNormal);

    if (!gl_FrontFacing)
    {
        N = -N;
        T = -T;
    }

    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(N, T));

    if (useTriplanar)
    {
        vec3 triPos = useWorldTriplanar ? WorldPos : LocalPos;
        vec3 triGeomNormal = useWorldTriplanar ? WorldNormal : normalize(LocalNormal);
        vec3 triP = vec3(triPos.x * UVScale.x, triPos.y * UVScale.y, triPos.z * UVScale.x) + vec3(UVOffset.x, UVOffset.y, 0.0);
        vec3 triWeights = getTriplanarWeights(triGeomNormal, triplanarSharpness);

        if (transparencyMode == 2 || transparencyMode == 3)
        {
            float alpha = (useAlbedoMap ? sampleTriplanar(albedoMap, triP, triWeights).a : 1.0) * albedoColor.a;
            if (alpha < alphaCutoff)
            {
                discard;
            }
        }

        vec3 worldNmap;
        if (useNormalMap)
        {
            vec3 blendedNorm = sampleTriplanarNormal(normalMap, triP, triWeights, triGeomNormal);
            worldNmap = useWorldTriplanar ? blendedNorm : normalize(normalMatrix * blendedNorm);
        }
        else
        {
            worldNmap = N;
        }

        gNormal = normalize(mat3(view) * worldNmap);
        gRoughness = useRoughnessMap ? sampleTriplanar(roughnessMap, triP, triWeights).r : roughness;
        gMetallic = useMetallicMap ? sampleTriplanar(metallicMap, triP, triWeights).r : metallic;
    }
    else
    {
        if (transparencyMode == 2 || transparencyMode == 3)
        {
            float alpha = (useAlbedoMap ? texture(albedoMap, TexCoords).a : 1.0) * albedoColor.a;
            if (alpha < alphaCutoff)
            {
                discard;
            }
        }

        vec3 worldNmap = getNormalFromMap(TexCoords, N, T, B);
        gNormal = normalize(mat3(view) * worldNmap);
        gRoughness = useRoughnessMap ? texture(roughnessMap, TexCoords).r : roughness;
        gMetallic = useMetallicMap ? texture(metallicMap, TexCoords).r : metallic;
    }
}