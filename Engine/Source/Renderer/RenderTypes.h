#pragma once
#include <cstdint>

namespace rv {

inline constexpr int MaxPointLights = 20;
inline constexpr int MaxSpotLights = 8;

enum class DebugBufferMode : uint8_t
{
    None = 0,
    Normal,
    Depth,
    Roughness,
    Metallic,
    SSAO,
    SSGI,
    SSR,
    DirectionalShadow,
    SpotShadow,
    Bloom,
    BrightPass,
    EntityID,
    ScreenColor,
    ContactShadows,
    PointShadow,
    MotionBlur
};

enum class ShadingMode : uint8_t
{
    Lit = 0,
    Unshaded
};

enum class TransparencyMode : uint8_t
{
    Opaque = 0,
    Alpha,
    AlphaScissor,
    DepthPrepass
};

enum class BlendMode : uint8_t
{
    Mix = 0,
    Add,
    Subtract,
    Multiply
};

enum class CullMode : uint8_t
{
    Back = 0,
    Front,
    Disabled
};

enum class BillboardMode : uint8_t
{
    Disabled = 0,
    Spherical,
    Cylindrical
};

struct CameraUBOData
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 invView;
    glm::mat4 invProjection;
    glm::vec3 camPos;
    float padding0;
    glm::vec2 windowSize;
    float nearPlane;
    float farPlane;
};

struct PointLightUBOData {
    glm::vec4 position;
    glm::vec4 colorIntensity;
    float radius;
    float shadowBias;
    float blurRadius;
    int shadowIndex;
};

struct DirectionalLightUBOData {
    glm::vec4 direction;
    glm::vec4 colorIntensity;
    float shadowBias;
    float blurRadius;
    float normalBias;
    int cascadeCount;
};

struct SpotLightUBOData {
    glm::vec4 position;
    glm::vec4 direction;
    glm::vec4 colorIntensity;
    glm::vec4 cutoffs;
    glm::mat4 lightSpaceMatrix;
};

struct LightUBOData {
    DirectionalLightUBOData dirLight;
    PointLightUBOData pointLights[MaxPointLights];
    SpotLightUBOData spotLights[MaxSpotLights];
    glm::mat4 cascadeMatrices[4];
    glm::vec4 cascadeSplits;
    int pointLightCount;
    int spotLightCount;
    int hasDirLight;
    float padding;
};

}
