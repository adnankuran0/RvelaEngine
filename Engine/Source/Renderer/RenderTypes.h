#pragma once
#include <cstdint>

namespace rv {

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
    glm::vec2 padding;
};

struct LightUBOData {
    DirectionalLightUBOData dirLight;
    PointLightUBOData pointLights[20];
    glm::mat4 lightSpaceMatrix;
    int pointLightCount;
    int hasDirLight;
    glm::vec2 padding;
};

}