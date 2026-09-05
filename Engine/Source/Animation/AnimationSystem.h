#pragma once
#include "Animation/AnimationEvent.h"
#include <entt/entt.h>
#include <string>
#include <vector>

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
    entt::entity ResolveAnimPath(entt::entity root, const std::string& path);

    std::vector<Animation::AnimationDispatchEvent> m_EventQueue;
    Scene& m_Scene;
};

}