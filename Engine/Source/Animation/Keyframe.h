#pragma once
#include "Animation/EaseType.h"

namespace rv::Animation
{

template <typename T>
struct Keyframe
{
    float time = 0.0f;
    T value{};
    EaseType ease = EaseType::Linear;

    Keyframe() = default;
    Keyframe(float t, const T& val, EaseType e = EaseType::Linear)
        : time(t), value(val), ease(e) {
    }
};

}