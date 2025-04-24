#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

#define MAX_POINT_LIGHTS 10
#define MAX_DIRECTIONAL_LIGHTS 1

// Material parameters
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D heightMap;

uniform bool useAlbedoMap;
uniform bool useNormalMap;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useAOMap;
uniform bool useHeightMap;

uniform vec3 albedoColor;
uniform float metallicValue;
uniform float roughnessValue;
uniform float aoValue;
uniform float heightScale;

// Lights
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int pointLightCount;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
    bool castShadows;
};
uniform DirectionalLight directionalLight;
uniform bool hasDirectionalLight;

// Camera
uniform vec3 camPos;

const float PI = 3.14159265359;

// Helper functions
vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    if (!useHeightMap) return texCoords;
    float height = texture(heightMap, texCoords).r; 
    vec2 p = viewDir.xy / max(viewDir.z, 0.01) * (height * heightScale);
    return texCoords - p;
}

vec3 getNormalFromMap() {
    if (!useNormalMap) return normalize(Normal);
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
    vec3 Q1 = dFdx(FragPos);
    vec3 Q2 = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);
    vec3 N = normalize(Normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = normalize(-cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 calculateDirectionalLight(DirectionalLight light, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(V + L);
    
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * light.color * light.intensity * NdotL;
}

void main() {
    // Temel bilgiler
    vec3 viewDir = normalize(camPos - FragPos);
    vec2 adjustedTexCoords = parallaxMapping(TexCoords, viewDir);

    // Material özellikleri
    vec4 albedoTex = useAlbedoMap ? texture(albedoMap, adjustedTexCoords) : vec4(albedoColor, 1.0);
    vec3 albedo = pow(albedoTex.rgb, vec3(2.2));
    float alpha = albedoTex.a;
    if (alpha < 0.1) discard;

    float metallic = useMetallicMap ? texture(metallicMap, adjustedTexCoords).r : metallicValue;
    float roughness = useRoughnessMap ? texture(roughnessMap, adjustedTexCoords).r : roughnessValue;
    float ao = useAOMap ? texture(aoMap, adjustedTexCoords).r : aoValue;

    // Normal ve F0
    vec3 N = getNormalFromMap();
    vec3 V = viewDir;
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Işık hesaplamaları
    vec3 Lo = vec3(0.0);
    
    // Directional light
    if (hasDirectionalLight) {
        Lo += calculateDirectionalLight(directionalLight, N, V, albedo, metallic, roughness, F0);
    }
    
    // Point lights
    for (int i = 0; i < pointLightCount; ++i) {
        vec3 L = normalize(pointLights[i].position - FragPos);
        vec3 H = normalize(V + L);
        float distance = length(pointLights[i].position - FragPos);
        
        // Radius desteği eklenmiş attenuation formülü
        float attenuation = 1.0 / (1.0 + (distance / pointLights[i].radius) * (distance / pointLights[i].radius));
        vec3 radiance = pointLights[i].color * attenuation * pointLights[i].intensity;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // Ambient ve tonemapping
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, alpha);
}
