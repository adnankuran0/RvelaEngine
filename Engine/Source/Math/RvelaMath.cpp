#include "rvelapch.h"
#define GLM_FORCE_INTRINSICS
#include "RvelaMath.h"

void DecomposeToEulerAngles(const glm::mat4& worldMatrix, glm::vec3& scale, glm::vec3& rotationDegrees, glm::vec3& translation)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat orientation;
    glm::decompose(worldMatrix, scale, orientation, translation, skew, perspective);
    rotationDegrees = glm::degrees(glm::eulerAngles(orientation));
}

glm::quat EulerToQuat(const glm::vec3& eulerDegrees) {
    glm::vec3 radians = glm::radians(eulerDegrees);
    return glm::quat(radians);
}

glm::vec3 QuatToEuler(const glm::quat& q) {
    return glm::degrees(glm::eulerAngles(q));
}
