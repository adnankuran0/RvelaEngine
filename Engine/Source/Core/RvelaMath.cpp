#include "RvelaMath.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

void DecomposeToEulerAngles(const glm::mat4& worldMatrix, glm::vec3& scale, glm::vec3& rotation, glm::vec3& translation)
{
    using namespace glm;

    mat4 LocalMatrix(worldMatrix);

    // Normalize the matrix to avoid floating-point inaccuracies.
    if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<float>()))
        return;

    // Extract translation
    translation = vec3(LocalMatrix[3]);
    LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

    // Extract scale
    vec3 Row[3];
    for (int i = 0; i < 3; ++i)
        Row[i] = vec3(LocalMatrix[i]);

    scale.x = length(Row[0]);
    Row[0] = normalize(Row[0]);
    scale.y = length(Row[1]);
    Row[1] = normalize(Row[1]);
    scale.z = length(Row[2]);
    Row[2] = normalize(Row[2]);

    // Extract rotation
    rotation.y = asin(-Row[0][2]); // Pitch
    if (cos(rotation.y) > 0.0001f) {
        rotation.x = atan2(Row[1][2], Row[2][2]); // Roll
        rotation.z = atan2(Row[0][1], Row[0][0]); // Yaw
    }
    else {
        // Gimbal lock case: cos(y) is close to 0
        rotation.x = atan2(-Row[2][0], Row[1][1]);
        rotation.z = 0;
    }

    // Convert radians to degrees
    rotation = degrees(rotation);
}

