#pragma once
#include <string>
#include "entt/entt.h"

namespace rv::Animation
{

enum class EventType
{
    Triggered,
    Started,
    Looped,
    Finished
};

struct AnimationEvent
{
    float time = 0.0f;
    std::string name;
    std::string parameter;

    bool operator<(const AnimationEvent& o) const { return time < o.time; }
};

struct AnimationDispatchEvent
{
    entt::entity entity{ entt::null };
    EventType type{ EventType::Triggered };
    std::string clipName;
    std::string eventName;
    std::string parameter;
};

}