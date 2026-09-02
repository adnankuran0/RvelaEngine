#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_float.hpp>
#include "Animation/InterpolationType.h"

namespace rv::Animation
{
inline glm::vec3 Interpolate(const glm::vec3& a, const glm::vec3& b, float alpha, InterpolationType type) {
    if (type == InterpolationType::STEP) return a;
    return glm::mix(a, b, alpha);
}

inline glm::quat Interpolate(const glm::quat& a, const glm::quat& b, float alpha, InterpolationType type) {
    if (type == InterpolationType::STEP) return a;
    if (type == InterpolationType::LINEAR) {
        return glm::slerp(a, b, alpha);
    }
    return glm::normalize(glm::lerp(a, b, alpha));

}

inline float PingPong(float value, float length)
{
    if (length <= 0.0f)
        return 0.0f;
    float v = std::fmod(value - length, length * 2.0f);
    if (v < 0.0f)
        v += length * 2.0f;
    return std::abs(v - length);
}
}