#shader vertex
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec2 aTexCoords;

#ifdef SKELETAL
layout(location = 4) in uvec4 aBoneIDs;
layout(location = 5) in vec4 aWeights;

const int MAX_BONES = 100;
uniform mat4 u_BoneMatrices[MAX_BONES];
#endif

#include "Common/Camera.glsl"
#include "Common/Lights.glsl"
#include "Common/Billboard.glsl"

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out vec3 Tangent;
out vec3 Bitangent;
out vec3 LocalPos;
out vec3 LocalNormal;

uniform mat4 model;
uniform vec2 UVScale;
uniform vec2 UVOffset;
uniform mat3 normalMatrix;

uniform int billboardMode;

void main()
{
    vec4 worldPos;
    vec3 N, T, B;

#ifdef SKELETAL
    mat4 skinMatrix = 
        u_BoneMatrices[aBoneIDs[0]] * aWeights[0] +
        u_BoneMatrices[aBoneIDs[1]] * aWeights[1] +
        u_BoneMatrices[aBoneIDs[2]] * aWeights[2] +
        u_BoneMatrices[aBoneIDs[3]] * aWeights[3];

    vec4 localPos     = skinMatrix * vec4(aPos, 1.0);
    mat3 skinMatrixIT = mat3(skinMatrix);
    vec3 localNormal  = skinMatrixIT * aNormal;
    vec3 localTangent = skinMatrixIT * aTangent;

    worldPos = model * localPos;
    N = normalize(normalMatrix * localNormal);
    T = normalize(mat3(model) * localTangent);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));
#else
    if (billboardMode == 0)
    {
        worldPos = model * vec4(aPos, 1.0);
        N = normalize(normalMatrix * aNormal);
        T = normalize(mat3(model) * aTangent);
        T = normalize(T - dot(T, N) * N);
        B = normalize(cross(N, T));
    }
    else
    {
        BillboardTransform bb = CalculateBillboard(billboardMode, model, view, aPos, camPos);
        worldPos = bb.worldPos;
        N = normalize(bb.normal);
        T = normalize(bb.tangent);
        B = normalize(bb.bitangent);
    }
#endif

    FragPos = worldPos.xyz;
    Normal = N;
    Tangent = T;
    Bitangent = B;
#ifdef SKELETAL
    LocalPos = localPos.xyz;
    LocalNormal = normalize(localNormal);
#else
    LocalPos = aPos;
    LocalNormal = normalize(aNormal);
#endif

    TexCoords = aTexCoords * UVScale + UVOffset;
    FragPosLightSpace = cascadeMatrices[0] * worldPos;

    gl_Position = projection * view * worldPos;
}

#shader fragment
#version 460 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;
in vec3 Tangent;
in vec3 Bitangent;
in vec3 LocalPos;
in vec3 LocalNormal;

#include "Common/Camera.glsl"
#include "Common/Lights.glsl"
#include "Common/Constants.glsl"
#include "Common/PBR.glsl"
#include "Common/Shadows.glsl"

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D metallicMap;
layout(binding = 3) uniform sampler2D roughnessMap;
layout(binding = 4) uniform sampler2D aoMap;
layout(binding = 5) uniform sampler2D heightMap;

uniform bool useAlbedoMap;
uniform bool useNormalMap;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useAOMap;
uniform bool useHeightMap;

uniform bool useTriplanar;
uniform bool useWorldTriplanar;
uniform float triplanarSharpness;
uniform vec2 UVScale;
uniform vec2 UVOffset;
uniform mat3 normalMatrix;

uniform vec4 albedoColor;
uniform vec3 emmisiveColor;
uniform float emmisiveIntensity;
uniform float metallicValue;
uniform float roughnessValue;
uniform float aoValue;
uniform float heightScale;
uniform float normalScale;
uniform float specularIntensity;

uniform int transparencyMode;
uniform float alphaCutoff;

uniform vec3 ambientColor;
uniform float ambientIntensity;

uniform int shadingMode;
uniform bool receiveShadows;

layout(binding = 6) uniform sampler2DArray shadowMap;
layout(binding = 7) uniform samplerCubeArray pointShadowMap;

layout(binding = 8) uniform samplerCube irradianceMap;
layout(binding = 9) uniform samplerCube prefilterMap;
layout(binding = 10) uniform sampler2D brdfLUT;

layout(binding = 11) uniform sampler2D ssaoTexture;
uniform bool useSSAO;

uniform bool useIBL;
uniform float iblIntensity;

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
    return vec4(
        sampX.rgb * weights.x + sampY.rgb * weights.y + sampZ.rgb * weights.z,
        sampX.a * weights.x + sampY.a * weights.y + sampZ.a * weights.z
    );
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

vec2 parallaxOcclusionMapping(vec2 texCoords, vec3 viewDirTS)
{
    if (!useHeightMap) return texCoords;

    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDirTS)));

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = normalize(viewDirTS).xy * -heightScale; 
    vec2 deltaTexCoords = P / numLayers;
    
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(heightMap, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentLayerDepth += layerDepth;
        currentDepthMapValue = texture(heightMap, currentTexCoords).r;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(heightMap, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth + 0.0001);
    
    return mix(currentTexCoords, prevTexCoords, weight);
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

float sampleSSAO()
{
    if (!useSSAO) return 1.0;

    vec2 screenUV = gl_FragCoord.xy / windowSize;
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoTexture, 0));

    float result = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            result += texture(ssaoTexture, screenUV + vec2(x, y) * texelSize).r;

    return result / 9.0;
}

void main()
{
    vec3 viewDir = normalize(camPos - FragPos);

    vec3 T = normalize(Tangent);
    vec3 N = normalize(Normal);

    if (!gl_FrontFacing)
    {
        N = -N;
        T = -T;
    }

    T = normalize(T - dot(T, N) * N);
    vec3 B = normalize(cross(N, T));

    vec4 albedoTex;
    vec3 Nmap;
    float metallic;
    float roughnessMapValue;
    float materialAO;

    if (useTriplanar)
    {
        vec3 triPos = useWorldTriplanar ? FragPos : LocalPos;
        vec3 triGeomNormal = useWorldTriplanar ? N : normalize(LocalNormal);
        
        vec3 triP = triPos * UVScale.x + vec3(UVOffset.x, UVOffset.y, 0.0);
        
        vec3 triWeights = getTriplanarWeights(triGeomNormal, triplanarSharpness);

        albedoTex = useAlbedoMap ? sampleTriplanar(albedoMap, triP, triWeights) : vec4(1.0);
        metallic = useMetallicMap ? sampleTriplanar(metallicMap, triP, triWeights).r : metallicValue;
        roughnessMapValue = useRoughnessMap ? sampleTriplanar(roughnessMap, triP, triWeights).r : 1.0;
        materialAO = useAOMap ? sampleTriplanar(aoMap, triP, triWeights).r : aoValue;

        if (useNormalMap)
        {
            vec3 blendedNorm = sampleTriplanarNormal(normalMap, triP, triWeights, triGeomNormal);
            Nmap = useWorldTriplanar ? blendedNorm : normalize(normalMatrix * blendedNorm);
        }
        else
        {
            Nmap = N;
        }
    }
    else
    {
        mat3 TBN = mat3(T, B, N);
        vec3 viewDirTS = transpose(TBN) * viewDir;
        vec2 mappedTexCoords = parallaxOcclusionMapping(TexCoords, viewDirTS);

        albedoTex = useAlbedoMap ? texture(albedoMap, mappedTexCoords) : vec4(1.0);
        metallic = useMetallicMap ? texture(metallicMap, mappedTexCoords).r : metallicValue;
        roughnessMapValue = useRoughnessMap ? texture(roughnessMap, mappedTexCoords).r : 1.0;
        materialAO = useAOMap ? texture(aoMap, mappedTexCoords).r : aoValue;
        Nmap = getNormalFromMap(mappedTexCoords, N, T, B);
    }

    vec4 fullAlbedo = albedoTex * albedoColor;
    vec3 albedo = fullAlbedo.rgb;
    float alpha = fullAlbedo.a;

    if (transparencyMode == 0) alpha = 1.0;
    else if (transparencyMode == 2) {
        if (alpha < alphaCutoff) discard;
        alpha = 1.0;
    }

    if (shadingMode == 1) {
        vec3 unshadedColor = albedo + (emmisiveColor * emmisiveIntensity);
        FragColor = vec4(unshadedColor, alpha);
        return; 
    }

    float roughness = clamp(roughnessValue * roughnessMapValue, 0.0, 1.0);
    float screenSpaceAO = transparencyMode == 1 ? 1.0 : sampleSSAO();
    float ao = materialAO * screenSpaceAO;

    vec3 V = normalize(camPos - FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);

    if (hasDirectionalLight == 1) {
        vec3 L = normalize(-directionalLight.direction.xyz);
        vec3 H = normalize(V + L);
        
        float NDF = DistributionGGX(Nmap, H, roughness);
        float G = GeometrySmith(Nmap, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(Nmap, V), 0.0) * max(dot(Nmap, L), 0.0) + 0.0001;
        vec3 specular = (numerator / denominator) * specularIntensity;
        float NdotL = max(dot(Nmap, L), 0.0);
        
        bool castShadows = directionalLight.direction.w > 0.5;
        float viewDepth = abs((view * vec4(FragPos, 1.0)).z);
        float shadow = (castShadows && receiveShadows) ?   
            calculateDirectionalShadow(shadowMap, FragPos, Nmap, L, viewDepth, directionalLight.shadowBias, directionalLight.normalBias, directionalLight.blurRadius) : 0.0;

        vec3 lightColor = directionalLight.colorIntensity.rgb;
        float lightIntensity = directionalLight.colorIntensity.a;

        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * lightColor * lightIntensity * NdotL;
    }

    for (int i = 0; i < pointLightCount; ++i) 
    {
        PointLight light = pointLights[i];
        vec3 L_vec = light.position.xyz - FragPos;
        float dist2 = dot(L_vec, L_vec);
        float radius2 = light.radius * light.radius;
        
        if (dist2 > radius2) continue;
        
        float distance = sqrt(dist2);
        vec3 L = L_vec / distance;
        vec3 H = normalize(V + L);
        
        float num = clamp(1.0 - (dist2 / radius2) * (dist2 / radius2), 0.0, 1.0);
        float attenuation = (num * num) / (dist2 + 1.0);
        
        float NDF = DistributionGGX(Nmap, H, roughness);
        float G = GeometrySmith(Nmap, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(Nmap, V), 0.0) * max(dot(Nmap, L), 0.0) + 0.0001;
        vec3 specular = (numerator / denominator) * specularIntensity;
        float NdotL = max(dot(Nmap, L), 0.0);
        
        bool castShadows = light.shadowIndex >= 0;
        float shadow = (castShadows && receiveShadows) ? 
            calculatePointLightShadow(pointShadowMap, light.shadowIndex, FragPos, light.position.xyz, light.radius, Nmap, light.shadowBias, light.blurRadius) : 0.0;

        vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.a * attenuation;
        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.0);
    if (useIBL) {
        vec3 R = normalize(reflect(-V, Nmap));
        float NdotV = max(dot(Nmap, V), 0.0);
        vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic); 

        vec3 irradiance = texture(irradianceMap, Nmap).rgb;
        vec3 diffuse = irradiance * albedo;

        float lod = roughness * 8.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, lod).rgb;
        vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y) * specularIntensity;
        ambient = (kD * diffuse + specular) * ao * iblIntensity;
    } else {
        ambient = ambientColor * ambientIntensity * albedo * ao;
    }
    
    FragColor = vec4(ambient + Lo + emmisiveColor * emmisiveIntensity, alpha);
}