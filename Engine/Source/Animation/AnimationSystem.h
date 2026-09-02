#pragma once

namespace rv {

class Scene;

class AnimationSystem
{
public:
    AnimationSystem(Scene& scene) : m_Scene(scene) {}
    void OnStart();
    void Update();

private:
    Scene& m_Scene;
};

}