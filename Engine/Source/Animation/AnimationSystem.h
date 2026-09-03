#pragma once
#include "Animation/AnimationEvent.h"

namespace rv {

class Scene;

class AnimationSystem
{
public:
    AnimationSystem(Scene& scene) : m_Scene(scene) {}
    void OnStart();
    void Update();
    std::vector<Animation::AnimationDispatchEvent> FlushEvents()
    {
        return std::move(m_EventQueue);
    }

private:
    std::vector<Animation::AnimationDispatchEvent> m_EventQueue;
    Scene& m_Scene;
};

}