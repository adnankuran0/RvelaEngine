#shader vertex
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec2 aTexCoords;

#include "Common/Billboard.glsl"

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out vec3 Tangent;
out vec3 Bitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec2 UVScale;
uniform vec2 UVOffset;

uniform mat4 lightSpaceMatrix;
uniform mat3 normalMatrix;

uniform int billboardMode;
uniform vec3 camPos;

void main()
{
    vec4 worldPos;
    vec3 N, T, B;

    if (billboardMode == 0)
    {
        worldPos = model * vec4(aPos, 1.0);
        N = normalize(normalMatrix * aNormal);
        T = normalize(mat3(model) * aTangent);
        T = normalize(T - dot(T, N) * N);
        B = cross(N, T);
    }
    else
    {
        BillboardTransform bb = CalculateBillboard(billboardMode, model, view, aPos, camPos);
        worldPos = bb.worldPos;
        N = bb.normal;
        T = bb.tangent;
        B = bb.bitangent;
    }

    FragPos = worldPos.xyz;
    Normal = N;
    Tangent = T;
    Bitangent = B;

    TexCoords = aTexCoords * UVScale + UVOffset;
    FragPosLightSpace = lightSpaceMatrix * worldPos;

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

layout(binding = 6) uniform sampler2D shadowMap;
layout(binding = 7) uniform samplerCubeArray pointShadowMap;
uniform mat4 lightSpaceMatrix;

layout(binding = 8) uniform samplerCube irradianceMap;
layout(binding = 9) uniform samplerCube prefilterMap;
layout(binding = 10) uniform sampler2D brdfLUT;

uniform bool useIBL;
uniform float iblIntensity;

#define MAX_POINT_LIGHTS 20
#define MAX_DIRECTIONAL_LIGHTS 1

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
    float falloff;
    bool castShadows;
    float shadowBias;
    float blurRadius;
    int shadowIndex;
};
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int pointLightCount;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
    bool castShadows;
    float shadowBias;
    float blurRadius;
};
uniform DirectionalLight directionalLight;
uniform bool hasDirectionalLight;

uniform vec3 camPos;

vec2 parallaxOcclusionMapping(vec2 texCoords, vec3 viewDirTS)
{
    if(!useHeightMap) return texCoords;

    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0,0.0,1.0), viewDirTS)));

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = normalize(viewDirTS).xy * -heightScale; 
    vec2 deltaTexCoords = P / numLayers;
    
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(heightMap, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
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

vec3 getNormalFromMap(vec2 texCoords) 
{
    if (!useNormalMap) return normalize(Normal);
    
    vec2 rg = texture(normalMap, texCoords).rg * 2.0 - 1.0;
    float b = sqrt(max(0.0, 1.0 - dot(rg, rg)));
    vec3 tangentNormal = vec3(rg, b);
    
    tangentNormal.xy *= normalScale;
    
    float tnLen = length(tangentNormal);
    if (tnLen < 0.001) return normalize(Normal);
    tangentNormal /= tnLen;
    
    vec3 N = normalize(Normal);
    vec3 T = normalize(Tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    mat3 TBN = mat3(T, B, N);
    vec3 result = TBN * tangentNormal;
    
    float rLen = length(result);
    if (rLen < 0.001) return N;
    return result / rLen;
}

void main()
{
    vec3 viewDir = normalize(camPos - FragPos);

    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);
    vec3 N = normalize(Normal);
    mat3 TBN = mat3(T, B, N);
    vec3 viewDirTS = transpose(TBN) * viewDir;

    vec2 mappedTexCoords = parallaxOcclusionMapping(TexCoords, viewDirTS);

    vec4 albedoTex = useAlbedoMap ? texture(albedoMap, mappedTexCoords) : vec4(1.0);
    vec4 fullAlbedo = albedoTex * albedoColor;
    vec3 albedo = fullAlbedo.rgb;
    float alpha = fullAlbedo.a;

    if (transparencyMode == 0)
    {
        alpha = 1.0;
    }
    else if (transparencyMode == 2)
    {
        if (alpha < alphaCutoff)
        {
            discard;
        }
        alpha = 1.0;
    }

    if (shadingMode == 1) 
    {
        vec3 unshadedColor = albedo + (emmisiveColor * emmisiveIntensity);
        FragColor = vec4(unshadedColor, alpha);
        return; 
    }

    float metallic = useMetallicMap ? texture(metallicMap, mappedTexCoords).r : metallicValue;
    float roughnessMapValue = useRoughnessMap ? texture(roughnessMap, mappedTexCoords).r : 1.0;
    float roughness = clamp(roughnessValue * roughnessMapValue, 0.0, 1.0);
    float ao = useAOMap ? texture(aoMap, mappedTexCoords).r : aoValue;

    vec3 V = normalize(camPos - FragPos);
    vec3 Nmap = getNormalFromMap(mappedTexCoords);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);

    if(hasDirectionalLight) {
        vec3 L = normalize(-directionalLight.direction);
        vec3 H = normalize(V + L);
        
        float NDF = DistributionGGX(Nmap, H, roughness);
        float G = GeometrySmith(Nmap, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(Nmap, V), 0.0) * max(dot(Nmap, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        specular *= specularIntensity;
        float NdotL = max(dot(Nmap, L), 0.0);
        float shadow = (directionalLight.castShadows && receiveShadows) ?   
            calculateDirectionalShadow(shadowMap, FragPosLightSpace, Nmap, L, directionalLight.shadowBias, directionalLight.blurRadius) : 0.0;

        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * 
              directionalLight.color * directionalLight.intensity * NdotL;
    }

    for(int i = 0; i < pointLightCount; ++i) 
    {
        PointLight light = pointLights[i];
        vec3 L_vec = light.position - FragPos;
        float dist2 = dot(L_vec, L_vec);
        float radius2 = light.radius * light.radius;
        
        if(dist2 > radius2) continue;
        
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
       
        vec3 specular = numerator / denominator;
        specular *= specularIntensity;
        float NdotL = max(dot(Nmap, L), 0.0);
        float shadow = (light.castShadows && receiveShadows) ? 
            calculatePointLightShadow(pointShadowMap, light.shadowIndex, FragPos, light.position, light.radius, Nmap, light.shadowBias, light.blurRadius) : 0.0;

        vec3 radiance = light.color * light.intensity * attenuation;
        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.0);

    if(useIBL)
    {
        vec3 R = normalize(reflect(-V, Nmap));
        
        float NdotV = max(dot(Nmap, V), 0.0);
        vec3 F = fresnelSchlick(NdotV, F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic; 

        vec3 irradiance = texture(irradianceMap, Nmap).rgb;
        vec3 diffuse = irradiance * albedo;

        const float MAX_REFLECTION_LOD = 4.0;
        float lod = roughness * MAX_REFLECTION_LOD;
        vec3 prefilteredColor = textureLod(prefilterMap, R, lod).rgb;
        
        vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
        specular *= specularIntensity;
        ambient = (kD * diffuse + specular) * ao * iblIntensity;
    }
    else
    {
        ambient = ambientColor * ambientIntensity * albedo * ao;
    }
    
    vec3 color = ambient + Lo + emmisiveColor * emmisiveIntensity;

    FragColor = vec4(color, alpha);
}