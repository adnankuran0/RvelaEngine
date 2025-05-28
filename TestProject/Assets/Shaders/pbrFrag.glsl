#version 400 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

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

// Shadows
uniform sampler2D shadowMap;
uniform samplerCubeArray pointShadowMap;
uniform mat4 lightSpaceMatrix;

// Lights
#define MAX_POINT_LIGHTS 20
#define MAX_DIRECTIONAL_LIGHTS 1

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
    float falloff;
    bool castShadows;
    int shadowIndex;
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
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
);

// Helper functions
vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    if (!useHeightMap) return texCoords;
    float height = texture(heightMap, texCoords).r; 
    vec2 p = viewDir.xy / viewDir.z * (height * heightScale);
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

// PBR functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Shadow calculations
float calculateDirectionalShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // Perspective divide and transform to [0,1] range
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Early exit if outside shadow frustum
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    // Dynamic depth bias (reduces Peter Panning)
    float bias = min(0.001 * (1.0 - dot(normal, lightDir)), 0.001f);


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

float calculatePointLightShadow(int index, vec3 fragPos, vec3 lightPos, float farPlane, vec3 normal) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    
    if(currentDepth > farPlane) return 0.0;
    
    float shadow = 0.0;
    float bias = max(0.05 * (1.0 - dot(normal, normalize(fragToLight))), 0.005);
    float diskRadius = 0.05;
    
    for(int i = 0; i < 16; ++i) {
        vec3 sampleDir = fragToLight;
        sampleDir.xy += poissonDisk[i] * diskRadius;
        
        float closestDepth = texture(pointShadowMap, vec4(sampleDir, index)).r * farPlane;
        shadow += (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
    }
    
    return shadow / 16.0;
}

void main()
{
    // Early alpha test
    vec4 albedoTex = useAlbedoMap ? texture(albedoMap, TexCoords) : vec4(albedoColor, 1.0);
    if(albedoTex.a < 0.1) discard;

    // Material properties
    vec3 albedo = pow(albedoTex.rgb, vec3(2.2));
    float metallic = useMetallicMap ? texture(metallicMap, TexCoords).r : metallicValue;
    float roughness = useRoughnessMap ? texture(roughnessMap, TexCoords).r : roughnessValue;
    float ao = useAOMap ? texture(aoMap, TexCoords).r : aoValue;

    // View direction and normal
    vec3 N = getNormalFromMap();
    vec3 V = normalize(camPos - FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Reflectance equation
    vec3 Lo = vec3(0.0);

    // Directional light
    if(hasDirectionalLight) {
        vec3 L = normalize(-directionalLight.direction);
        vec3 H = normalize(V + L);
        
        // Cook-Torrance BRDF
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
        float shadow = directionalLight.castShadows ? 
            calculateDirectionalShadow(FragPosLightSpace, N, L) : 0.0;

        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * 
              directionalLight.color * directionalLight.intensity * NdotL;
    }

    // Point lights
    for(int i = 0; i < pointLightCount; ++i) 
    {
        PointLight light = pointLights[i];
        vec3 L = light.position - FragPos;
        float distance = length(L);
        
        if(distance > light.radius) continue;
        
        L = normalize(L);
        vec3 H = normalize(V + L);
        
        // Attenuation 
        float attenuation = 1.0 / (1.0 + light.falloff * distance * distance);
        attenuation *= 1.0 - smoothstep(light.radius * 0.75, light.radius, distance);
        
        // Cook-Torrance BRDF
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
        float shadow = light.castShadows ? 
            calculatePointLightShadow(light.shadowIndex, FragPos, light.position, light.radius, N) : 0.0;

        vec3 radiance = light.color * light.intensity * attenuation;
        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    // Tone mapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, albedoTex.a);
}