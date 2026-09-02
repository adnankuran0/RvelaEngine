#pragma once
#include "Animation/EaseType.h"

namespace rv::Animation
{

template <typename T>
struct Keyframe
{
    float time = 0.0f;
    T value{};
    EaseType ease = EaseType::LINEAR;

    Keyframe() = default;
    Keyframe(float t, const T& val, EaseType e = EaseType::LINEAR)
        : time(t), value(val), ease(e) {
    }
};

}