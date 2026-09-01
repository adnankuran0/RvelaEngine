#pragma once

namespace rv {

class Scene;

class AnimationSystem
{
public:
    AnimationSystem(Scene& scene) : m_Scene(scene) {}
    void Update();
    Scene& m_Scene;
};

}