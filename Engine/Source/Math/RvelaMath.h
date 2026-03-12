#pragma once
#include <glm/glm.hpp>
#include "assimp/matrix4x4.h"
#include "Jolt/Jolt.h"
#include "Jolt/Core/Color.h"
#include "Jolt/Math/MathTypes.h"
#include <glm/gtc/quaternion.hpp>

namespace rv::math {

static inline JPH::Vec3 ToJoltVec3(const glm::vec3& v) 
{
    return JPH::Vec3(v.x, v.y, v.z);
}

static inline JPH::RVec3 ToJoltRVec3(const glm::vec3& v) 
{
    return JPH::RVec3((JPH::Real)v.x, (JPH::Real)v.y, (JPH::Real)v.z);
}

static inline JPH::Quat ToJoltQuat(const glm::quat& q) 
{
    return JPH::Quat(q.x, q.y, q.z, q.w);
}

static inline glm::vec3 FromJoltVec3(const JPH::Vec3& v) 
{
    return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
}

static inline glm::vec3 FromJoltRVec3(const JPH::RVec3& v) 
{
    return glm::vec3((float)v.GetX(), (float)v.GetY(), (float)v.GetZ());
}

static inline glm::quat FromJoltQuat(const JPH::Quat& q) 
{
    return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

static inline glm::vec4 FromJoltColor(const JPH::Color& c)
{
    return glm::vec4(c.r,c.g,c.b,c.a);
}

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