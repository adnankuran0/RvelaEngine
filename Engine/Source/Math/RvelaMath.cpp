#include "rvelapch.h"
#define GLM_FORCE_INTRINSICS
#include "RvelaMath.h"
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

void rv::math::DecomposeToQuat(const glm::mat4& transform,
    glm::vec3& outScale, glm::quat& outRotation, glm::vec3& outTranslation)
{
    outTranslation = glm::vec3(transform[3]);

    glm::vec3 col0(transform[0]);
    glm::vec3 col1(transform[1]);
    glm::vec3 col2(transform[2]);

    outScale.x = glm::length(col0);
    outScale.y = glm::length(col1);
    outScale.z = glm::length(col2);

    if (glm::determinant(glm::mat3(transform)) < 0.0f)
        outScale.x = -outScale.x;

    glm::mat3 rotMat(
        col0 / (outScale.x != 0.0f ? outScale.x : 1.0f),
        col1 / (outScale.y != 0.0f ? outScale.y : 1.0f),
        col2 / (outScale.z != 0.0f ? outScale.z : 1.0f));

    outRotation = glm::normalize(glm::quat_cast(rotMat));
}

bool rv::math::RayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
        const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
        float& t)
{
    const float EPSILON = 1e-8f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;

    glm::vec3 h = cross(rayDir, edge2);
    float a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;

    float f = 1.0f / a;
    glm::vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    glm::vec3 q = cross(s, edge1);
    float v = f * dot(rayDir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * dot(edge2, q);
    return t > EPSILON;
}

glm::vec3 rv::math::ScreenPosToWorldRay(const glm::vec2& mousePos, const glm::vec2& viewportSize,
    const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
{
    float x = (2.0f * mousePos.x) / viewportSize.x - 1.0f;
    float y = 1.0f - (2.0f * mousePos.y) / viewportSize.y;

    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projectionMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec4 rayWorld = glm::inverse(viewMatrix) * rayEye;
    return glm::normalize(glm::vec3(rayWorld));
}

void rv::math::DecomposeToEulerAngles(const glm::mat4& worldMatrix, glm::vec3& scale, glm::vec3& rotationDegrees, glm::vec3& translation)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat orientation;
    glm::decompose(worldMatrix, scale, orientation, translation, skew, perspective);
    rotationDegrees = glm::degrees(glm::eulerAngles(orientation));
}

glm::quat rv::math::EulerToQuat(const glm::vec3& eulerDegrees) {
    glm::vec3 radians = glm::radians(eulerDegrees);
    return glm::quat(radians);
}

glm::vec3 rv::math::QuatToEuler(const glm::quat& q) {
    return glm::degrees(glm::eulerAngles(q));
}

glm::mat4 rv::math::ConvertToGlmMatrix(const aiMatrix4x4& from)
{
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}
