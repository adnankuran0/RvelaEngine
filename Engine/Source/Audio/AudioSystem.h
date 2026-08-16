#pragma once
#include <vector>
#include "entt/entt.h"

namespace rv {

class Scene; // forward declaration

class AudioSystem
{
public:
    AudioSystem(Scene& scene) : m_Scene(scene) { BindCallbacks(); }

    void OnStart();
    void Update();


private:
    void BindCallbacks();

    void OnAudioEmitterDestructed(entt::registry& reg, entt::entity e);

    Scene& m_Scene;

};

}