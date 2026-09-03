#pragma once
#include <vector>
#include "entt/entt.h"
#include "AudioEvent.h"

namespace rv {

class Scene;

class AudioSystem
{
public:
    AudioSystem(Scene& scene) : m_Scene(scene) { BindCallbacks(); }

    void OnStart();
    void Update();

    std::vector<Audio::AudioDispatchEvent> FlushEvents()
    {
        return std::move(m_EventQueue);
    }

private:
    void BindCallbacks();
    void OnAudioEmitterDestructed(entt::registry& reg, entt::entity e);

    std::vector<Audio::AudioDispatchEvent> m_EventQueue;
    Scene& m_Scene;
};

}