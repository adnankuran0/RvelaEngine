#pragma once

#include <glm/glm.hpp>
#include "assimp/matrix4x4.h"

void DecomposeToEulerAngles(const glm::mat4& worldMatrix, glm::vec3& scale, glm::vec3& rotation, glm::vec3& translation);
inline glm::mat4 ConvertToGlmMatrix(const aiMatrix4x4& from)
{
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

glm::quat EulerToQuat(const glm::vec3& eulerDegrees);
glm::vec3 QuatToEuler(const glm::quat& q);