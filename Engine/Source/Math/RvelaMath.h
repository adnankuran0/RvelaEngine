#pragma once

#include <glm/glm.hpp>

void DecomposeToEulerAngles(const glm::mat4& worldMatrix, glm::vec3& scale, glm::vec3& rotation, glm::vec3& translation);


glm::quat EulerToQuat(const glm::vec3& eulerDegrees);
glm::vec3 QuatToEuler(const glm::quat& q);