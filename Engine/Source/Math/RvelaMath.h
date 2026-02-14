#pragma once
#include <glm/glm.hpp>
#include "assimp/matrix4x4.h"


void DecomposeToEulerAngles(const glm::mat4& worldMatrix, glm::vec3& scale, glm::vec3& rotation, glm::vec3& translation);
glm::mat4 ConvertToGlmMatrix(const aiMatrix4x4& from);

glm::quat EulerToQuat(const glm::vec3& eulerDegrees);
glm::vec3 QuatToEuler(const glm::quat& q);