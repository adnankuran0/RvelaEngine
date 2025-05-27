#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

#define MAX_POINT_LIGHTS 20
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

uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

uniform samplerCube pointShadowMap;
uniform float pointLightFarPlane;


// Lights
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
    bool castShadows;
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

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // Perspective divide and transform to [0,1] range
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Early exit if outside shadow frustum
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    // Dynamic depth bias (reduces Peter Panning)
    float bias = min(0.005 * (1.0 - dot(normal, lightDir)), 0.005f);


    // PCF filtering (5x5 kernel for smoother shadows)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias) > closestDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;

    return shadow;
}

float calculatePointLightShadow(int index, vec3 fragPos, vec3 lightPos, float farPlane) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float shadow = 0.0;

    float bias = 0.1;
    float samples = 20.0;
    float diskRadius = (1.0 + (currentDepth / farPlane)) / 25.0;

    for (float i = 0.0; i < samples; ++i) {
        // Örnekleme için rasgele yönler, basit olarak küresel örnekleme (örnekleme yöntemi geliştirilebilir)
        vec3 sampleOffset = normalize(vec3(
            cos(i * 3.14159 * 2.0 / samples),
            sin(i * 3.14159 * 2.0 / samples),
            cos(i * 3.14159 / samples)
        )) * diskRadius;

        float closestDepth = texture(pointShadowMap, fragToLight + sampleOffset).r;
        closestDepth *= farPlane; // depth cubemap'lar normalize derinlik içerir, onu gerçek derinliğe çevirdik
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    shadow /= samples;
    if (currentDepth > farPlane) // ışık menzili dışında shadow yok
        shadow = 0.0;

    return shadow;
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
    if (hasDirectionalLight) 
    {
        vec3 L = normalize(-directionalLight.direction);
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

        vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
        float shadow = directionalLight.castShadows ? calculateShadow(fragPosLightSpace, N, L) : 0.0;

        vec3 lightColor = directionalLight.color * directionalLight.intensity;
        vec3 lighting = (kD * albedo / PI + specular) * lightColor * NdotL;
        Lo += (1.0 - shadow) * lighting;
    }
    
    // Point lights
    for (int i = 0; i < pointLightCount; ++i) 
    {
        PointLight light = pointLights[i];
        vec3 L = light.position - FragPos;
        float distance = length(L);
        if (distance > light.radius)
            continue;

        L = normalize(L);
        vec3 H = normalize(V + L);

        float attenuation = 1.0 - (distance / light.radius);
        attenuation = clamp(attenuation, 0.0, 1.0);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);

        float shadow = light.castShadows ? calculatePointLightShadow(i, FragPos, light.position, pointLightFarPlane) : 0.0;

        vec3 radiance = light.color * light.intensity * attenuation;
        Lo += (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadow);
    }
    
    // Ambient ve tonemapping
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, alpha);
}