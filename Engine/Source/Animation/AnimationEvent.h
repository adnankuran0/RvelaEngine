#pragma once
#include <string>

namespace rv::Animation
{

struct AnimationEvent
{
    float time = 0.0f;
    std::string name;
    std::string parameter;

    AnimationEvent() = default;
    AnimationEvent(float t, const std::string& n, const std::string& p = "")
        : time(t), name(n), parameter(p) {
    }

    bool operator<(const AnimationEvent& o) const { return time < o.time; }
};

}