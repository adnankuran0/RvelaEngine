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

out vec2 TexCoords;

uniform mat4 model;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceMatrix;
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

    TexCoords = UV * UVScale + UVOffset;

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
    vec4 worldPos = model * skinnedPos;
    if (POSITION == vec4(0.0))
        gl_Position = lightSpaceMatrix * worldPos;
    else
        gl_Position = POSITION;
#else
    BillboardTransform bb = CalculateBillboard(billboardMode, model, view, VERTEX, camPos);
    if (POSITION == vec4(0.0))
        gl_Position = lightSpaceMatrix * bb.worldPos;
    else
        gl_Position = POSITION;
#endif
}

#shader fragment
#version 460 core
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D albedoMap;
uniform bool useAlbedoMap;
uniform vec4 albedoColor;
uniform int transparencyMode;
uniform float alphaCutoff;

void main() 
{
    if (transparencyMode == 1)
        discard;

    if (transparencyMode == 2 || transparencyMode == 3) 
    {
        float alpha = (useAlbedoMap ? texture(albedoMap, TexCoords).a : 1.0) * albedoColor.a;
        if (alpha < alphaCutoff)
            discard;
    }
}
