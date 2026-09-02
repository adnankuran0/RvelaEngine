#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <cmath>
#include <algorithm>
#include "Animation/InterpolationType.h"
#include "Animation/EaseType.h"

namespace rv::Animation
{

    inline float ApplyEase(float t, EaseType ease)
    {
        t = std::clamp(t, 0.0f, 1.0f);

        constexpr float PI = 3.14159265358979323846f;

        switch (ease)
        {
        case EaseType::LINEAR:
            return t;

        case EaseType::EASE_IN:
            return t * t;

        case EaseType::EASE_OUT:
            return t * (2.0f - t);

        case EaseType::EASE_IN_OUT:
            return (t < 0.5f) ? (2.0f * t * t) : (-1.0f + (4.0f - 2.0f * t) * t);

        case EaseType::EASE_OUT_IN:
            if (t < 0.5f)
            {
                float sub = t * 2.0f;
                return 0.5f * sub * (2.0f - sub);
            }
            else
            {
                float sub = (t - 0.5f) * 2.0f;
                return 0.5f + 0.5f * (sub * sub);
            }

        case EaseType::ZERO:
            return (t >= 1.0f) ? 1.0f : 0.0f;
        }

        return t;
    }

inline glm::vec3 Interpolate(const glm::vec3& a, const glm::vec3& b, float alpha, InterpolationType type)
{
    if (type == InterpolationType::STEP) return a;
    return glm::mix(a, b, alpha);
}

inline glm::quat Interpolate(const glm::quat& a, const glm::quat& b, float alpha, InterpolationType type)
{
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