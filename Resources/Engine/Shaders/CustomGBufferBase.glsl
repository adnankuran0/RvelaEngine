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
#endif

#include "Common/Camera.glsl"

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 WorldPos;
out vec3 WorldNormal;
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
#define NEAR_PLANE nearPlane
#define FAR_PLANE farPlane

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

    vec4 skinnedPos = skinMatrix * vec4(VERTEX, 1.0);
    mat3 skinMatrixIT = mat3(skinMatrix);
    vec3 skinnedNormal = skinMatrixIT * NORMAL;
    vec3 skinnedTangent = skinMatrixIT * TANGENT;

    worldPos = model * skinnedPos;
    calculatedViewNormal = mat3(view) * (normalMatrix * skinnedNormal);

    N = normalize(normalMatrix * skinnedNormal);
    T = normalize(mat3(model) * skinnedTangent);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));

    LocalPos = skinnedPos.xyz;
    LocalNormal = normalize(skinnedNormal);
#else
    worldPos = model * vec4(VERTEX, 1.0);
    calculatedViewNormal = mat3(view) * (normalMatrix * NORMAL);
    N = normalize(normalMatrix * NORMAL);
    T = normalize(mat3(model) * TANGENT);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));

    LocalPos = VERTEX;
    LocalNormal = normalize(NORMAL);
#endif

    FragPos = vec3(view * worldPos);
    Normal = calculatedViewNormal;
    WorldPos = worldPos.xyz;
    WorldNormal = N;
    Tangent = T;
    Bitangent = B;
    TexCoords = UV * UVScale + UVOffset;

    if (POSITION == vec4(0.0))
    {
        gl_Position = projection * view * worldPos;
    }
    else
    {
        gl_Position = POSITION;
    }
}

#shader fragment
#version 460 core

layout(location = 0) out vec3 gNormal;
layout(location = 1) out float gRoughness;
layout(location = 2) out float gMetallic;
layout(location = 3) out uint gEntityID;

uniform uint u_EntityID;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 WorldNormal;
in vec3 Tangent;
in vec3 Bitangent;
in vec3 LocalPos;
in vec3 LocalNormal;

#include "Common/Camera.glsl"

uniform mat4 model;
uniform mat3 normalMatrix;
uniform float u_Time;
uniform vec2 UVScale;
uniform vec2 UVOffset;

#define TIME u_Time
#define MODEL_MATRIX model
#define MODEL model
#define VIEW_MATRIX view
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
#define VIEW normalize(camPos - WorldPos)

layout(binding = 16) uniform sampler2D u_EngineScreenTexture;
layout(binding = 17) uniform sampler2D u_EngineDepthTexture;
layout(binding = 18) uniform sampler2D u_EngineNormalTexture;

#define SCREEN_TEXTURE u_EngineScreenTexture
#define DEPTH_TEXTURE u_EngineDepthTexture
#define NORMAL_TEXTURE u_EngineNormalTexture

// === USER_GLOBALS_FRAGMENT ===

void main()
{
    vec3 VERTEX = WorldPos;
    vec3 NORMAL = WorldNormal;
    vec3 TANGENT = Tangent;
    vec3 BINORMAL = Bitangent;
    vec3 BITANGENT = Bitangent;
    vec2 UV = TexCoords;
    vec4 COLOR = vec4(1.0);

    vec3 ALBEDO = vec3(1.0);
    float ALPHA = 1.0;
    float ALPHA_SCISSOR = 0.0;
    float METALLIC = 0.0;
    float ROUGHNESS = 1.0;
    float SPECULAR = 0.5;
    vec3 EMISSION = vec3(0.0);
    float AO = 1.0;
    vec3 NORMAL_MAP = vec3(0.5, 0.5, 1.0);
    float NORMAL_MAP_DEPTH = 1.0;

    mat3 v_TBN = mat3(Tangent, Bitangent, WorldNormal);

    // === USER_FRAGMENT_CODE ===

    if (ALPHA_SCISSOR > 0.0 && ALPHA < ALPHA_SCISSOR)
    {
        discard;
    }

    vec3 finalWorldNormal = WorldNormal;
    if (NORMAL_MAP != vec3(0.5, 0.5, 1.0))
    {
        vec3 nMap = NORMAL_MAP * 2.0 - 1.0;
        nMap.xy *= NORMAL_MAP_DEPTH;
        finalWorldNormal = normalize(v_TBN * nMap);
    }
    else if (NORMAL != WorldNormal)
    {
        finalWorldNormal = normalize(NORMAL);
    }

    // View-space normal for G-Buffer attachment 0
    gNormal = normalize(mat3(view) * finalWorldNormal);
    gRoughness = clamp(ROUGHNESS, 0.0, 1.0);
    gMetallic = clamp(METALLIC, 0.0, 1.0);
    gEntityID = u_EntityID;
}
