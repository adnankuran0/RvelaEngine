#pragma once
#include <glm/glm.hpp>
#include "assimp/matrix4x4.h"

namespace rv::math {

bool RayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
    float& t);
glm::vec3 ScreenPosToWorldRay(const glm::vec2& mousePos, const glm::vec2& viewportSize,
    const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);

void DecomposeToEulerAngles(const glm::mat4& worldMatrix, glm::vec3& scale, glm::vec3& rotation, glm::vec3& translation);
glm::mat4 ConvertToGlmMatrix(const aiMatrix4x4& from);

glm::quat EulerToQuat(const glm::vec3& eulerDegrees);
glm::vec3 QuatToEuler(const glm::quat& q);

}