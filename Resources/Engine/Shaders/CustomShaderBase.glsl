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

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out vec3 Tangent;
out vec3 Bitangent;
out vec3 LocalPos;
out vec3 LocalNormal;

uniform mat4 model;
uniform mat3 normalMatrix;
uniform float u_Time;
uniform vec2 UVScale;
uniform vec2 UVOffset;

#define TIME u_Time
#define MODEL_MATRIX model
#define MODEL model
#define VIEW_MATRIX view
#define VIEW view
#define PROJECTION_MATRIX projection
#define PROJECTION projection
#define INV_VIEW_MATRIX invView
#define INV_VIEW invView
#define INV_PROJECTION_MATRIX invProjection
#define INV_PROJECTION invProjection
#define CAMERA_POSITION camPos
#define CAMERA_POS camPos
#define VIEWPORT_SIZE windowSize

// === USER_GLOBALS_VERTEX ===

void main()
{
    vec3 VERTEX = aPos;
    vec3 NORMAL = aNormal;
    vec3 TANGENT = aTangent;
    vec3 BINORMAL = normalize(cross(aNormal, aTangent));
    vec3 BITANGENT = BINORMAL;
    vec2 UV = aTexCoords;
    vec4 COLOR = vec4(1.0);
    vec4 POSITION = vec4(0.0);

    // === USER_VERTEX_CODE ===

    vec4 worldPos;
    vec3 N, T, B;
#ifdef SKELETAL
    mat4 skinMatrix =
        u_BoneMatrices[aBoneIDs[0]] * aWeights[0] +
        u_BoneMatrices[aBoneIDs[1]] * aWeights[1] +
        u_BoneMatrices[aBoneIDs[2]] * aWeights[2] +
        u_BoneMatrices[aBoneIDs[3]] * aWeights[3];
    vec4 localPos     = skinMatrix * vec4(VERTEX, 1.0);
    mat3 skinMatrixIT = mat3(skinMatrix);
    vec3 localNormal  = skinMatrixIT * NORMAL;
    vec3 localTangent = skinMatrixIT * TANGENT;
    worldPos = model * localPos;
    N = normalize(normalMatrix * localNormal);
    T = normalize(mat3(model) * localTangent);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));
#else
    worldPos = model * vec4(VERTEX, 1.0);
    N = normalize(normalMatrix * NORMAL);
    T = normalize(mat3(model) * TANGENT);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));
#endif

    FragPos = worldPos.xyz;
    Normal = N;
    Tangent = T;
    Bitangent = B;
    LocalPos = VERTEX;
    LocalNormal = normalize(NORMAL);
    TexCoords = UV * UVScale + UVOffset;
    FragPosLightSpace = cascadeMatrices[0] * worldPos;

    vec4 defaultPosition = projection * view * worldPos;
    if (POSITION == vec4(0.0))
    {
        gl_Position = defaultPosition;
    }
    else
    {
        gl_Position = POSITION;
    }
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

uniform mat4 model;
uniform mat3 normalMatrix;
uniform float u_Time;

#define TIME u_Time
#define MODEL_MATRIX model
#define MODEL model
#define VIEW_MATRIX view
#define VIEW view
#define PROJECTION_MATRIX projection
#define PROJECTION projection
#define INV_VIEW_MATRIX invView
#define INV_VIEW invView
#define INV_PROJECTION_MATRIX invProjection
#define INV_PROJECTION invProjection
#define CAMERA_POSITION camPos
#define CAMERA_POS camPos
#define VIEWPORT_SIZE windowSize
#define FRAGCOORD gl_FragCoord
#define FRONT_FACING gl_FrontFacing
#define NEAR_PLANE nearPlane
#define FAR_PLANE farPlane
#define SCREEN_UV (gl_FragCoord.xy / windowSize)

// Global lighting uniforms
uniform vec3 ambientColor;
uniform float ambientIntensity;
uniform bool receiveShadows;

layout(binding = 6) uniform sampler2DArray shadowMap;
layout(binding = 7) uniform samplerCubeArray pointShadowMap;
layout(binding = 13) uniform sampler2DArray spotShadowMap;
layout(binding = 8) uniform samplerCube irradianceMap;
layout(binding = 9) uniform samplerCube prefilterMap;
layout(binding = 10) uniform sampler2D brdfLUT;
layout(binding = 11) uniform sampler2D ssaoTexture;
uniform bool useSSAO;
uniform bool useIBL;
uniform float iblIntensity;

// Engine-supplied textures
layout(binding = 14) uniform sampler2D u_EngineScreenTexture;
layout(binding = 15) uniform sampler2D u_EngineDepthTexture;
layout(binding = 16) uniform sampler2D u_EngineNormalTexture;

#define SCREEN_TEXTURE u_EngineScreenTexture
#define DEPTH_TEXTURE u_EngineDepthTexture
#define NORMAL_TEXTURE u_EngineNormalTexture

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

// === USER_GLOBALS_FRAGMENT ===

void main()
{
    vec3 VERTEX = FragPos;
    vec3 NORMAL = normalize(Normal);
    vec3 TANGENT = normalize(Tangent);
    if (!gl_FrontFacing)
    {
        NORMAL = -NORMAL;
        TANGENT = -TANGENT;
    }
    vec3 BINORMAL = normalize(Bitangent);
    vec3 BITANGENT = BINORMAL;
    vec2 UV = TexCoords;

    vec3 ALBEDO = vec3(1.0);
    float ALPHA = 1.0;
    float ALPHA_SCISSOR = 0.0;
    float METALLIC = 0.0;
    float ROUGHNESS = 1.0;
    float SPECULAR = 0.5;
    vec3 EMISSION = vec3(0.0);
    float AO = 1.0;
    vec3 NORMAL_MAP = vec3(0.0, 0.0, 1.0);

    // === USER_FRAGMENT_CODE ===

    if (ALPHA_SCISSOR > 0.0 && ALPHA < ALPHA_SCISSOR) discard;

#ifdef RVELA_UNLIT
    // --- Unlit Shading Output ---
    FragColor = vec4(ALBEDO + EMISSION, ALPHA);
#else
    // --- Lit PBR Lighting Pipeline ---
    vec3 N = NORMAL;
    vec3 T = normalize(TANGENT - dot(TANGENT, N) * N);
    vec3 B = normalize(cross(N, T));
    vec3 Nmap = N;
    if (dot(NORMAL_MAP - vec3(0.0, 0.0, 1.0), NORMAL_MAP - vec3(0.0, 0.0, 1.0)) > 0.0001)
    {
        Nmap = normalize(mat3(T, B, N) * normalize(NORMAL_MAP));
    }

    float roughness = clamp(ROUGHNESS, 0.04, 1.0);
    float screenSpaceAO = sampleSSAO();
    float ao = AO * screenSpaceAO;
    vec3 V = normalize(camPos - FragPos);
    vec3 F0 = mix(vec3(0.04), ALBEDO, METALLIC);
    vec3 Lo = vec3(0.0);

    if (hasDirectionalLight == 1) {
        vec3 L = normalize(-directionalLight.direction.xyz);
        vec3 H = normalize(V + L);
        float NDF = DistributionGGX(Nmap, H, roughness);
        float G = GeometrySmith(Nmap, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - METALLIC);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(Nmap, V), 0.0) * max(dot(Nmap, L), 0.0) + 0.0001;
        vec3 spec = (numerator / denominator) * SPECULAR;
        float NdotL = max(dot(Nmap, L), 0.0);
        bool castShadows = directionalLight.direction.w > 0.5;
        float viewDepth = abs((view * vec4(FragPos, 1.0)).z);
        float shadow = (castShadows && receiveShadows) ?
            calculateDirectionalShadow(shadowMap, FragPos, Nmap, L, viewDepth, directionalLight.shadowBias, directionalLight.normalBias, directionalLight.blurRadius) : 0.0;
        vec3 lightColor = directionalLight.colorIntensity.rgb;
        float lightIntensity = directionalLight.colorIntensity.a;
        Lo += (1.0 - shadow) * (kD * ALBEDO / PI + spec) * lightColor * lightIntensity * NdotL;
    }

    for (int i = 0; i < pointLightCount; ++i) {
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
        vec3 kD = (vec3(1.0) - kS) * (1.0 - METALLIC);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(Nmap, V), 0.0) * max(dot(Nmap, L), 0.0) + 0.0001;
        vec3 spec = (numerator / denominator) * SPECULAR;
        float NdotL = max(dot(Nmap, L), 0.0);
        bool castShadows = light.shadowIndex >= 0;
        float shadow = (castShadows && receiveShadows) ?
            calculatePointLightShadow(pointShadowMap, light.shadowIndex, FragPos, light.position.xyz, light.radius, Nmap, light.shadowBias, light.blurRadius) : 0.0;
        vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.a * attenuation;
        Lo += (1.0 - shadow) * (kD * ALBEDO / PI + spec) * radiance * NdotL;
    }

    for (int i = 0; i < spotLightCount; ++i) {
        SpotLight light = spotLights[i];
        vec3 L_vec = light.position.xyz - FragPos;
        float dist2 = dot(L_vec, L_vec);
        float radius2 = light.position.w * light.position.w;
        if (dist2 > radius2) continue;
        float distance = sqrt(dist2);
        vec3 L = L_vec / distance;
        float theta = dot(L, normalize(-light.direction.xyz));
        float innerCutoff = light.cutoffs.x;
        float outerCutoff = light.cutoffs.y;
        if (theta < outerCutoff) continue;
        float epsilon = innerCutoff - outerCutoff;
        float spotFactor = clamp((theta - outerCutoff) / max(0.0001, epsilon), 0.0, 1.0);
        float num = clamp(1.0 - (dist2 / radius2) * (dist2 / radius2), 0.0, 1.0);
        float attenuation = (num * num) / (dist2 + 1.0) * spotFactor;

        vec3 H = normalize(V + L);
        float NDF = DistributionGGX(Nmap, H, roughness);
        float G = GeometrySmith(Nmap, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - METALLIC);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(Nmap, V), 0.0) * max(dot(Nmap, L), 0.0) + 0.0001;
        vec3 spec = (numerator / denominator) * SPECULAR;
        float NdotL = max(dot(Nmap, L), 0.0);
        int sIndex = int(light.direction.w);
        float shadow = (sIndex >= 0 && receiveShadows) ?
            calculateSpotLightShadow(spotShadowMap, sIndex, FragPos, light.lightSpaceMatrix, Nmap, light.cutoffs.z, light.cutoffs.w) : 0.0;
        vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.a * attenuation;
        Lo += (1.0 - shadow) * (kD * ALBEDO / PI + spec) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.0);
    if (useIBL) {
        vec3 R = normalize(reflect(-V, Nmap));
        float NdotV = max(dot(Nmap, V), 0.0);
        vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - METALLIC);
        vec3 irradiance = texture(irradianceMap, Nmap).rgb;
        vec3 diffuse = irradiance * ALBEDO;
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        vec3 spec = prefilteredColor * (F * brdf.x + brdf.y) * SPECULAR;
        ambient = (kD * diffuse + spec) * ao * iblIntensity;
    } else {
        ambient = ambientColor * ambientIntensity * ALBEDO * ao;
    }

    FragColor = vec4(ambient + Lo + EMISSION, ALPHA);
#endif
}
