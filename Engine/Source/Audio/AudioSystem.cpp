#include "rvelapch.h"
#include "AudioSystem.h"
#include "Scene/Scene.h"
#include "AudioManager.h"

using namespace rv;

void AudioSystem::Update()
{
    auto& reg = m_Scene.GetRegistry();
    auto view = reg.view<AudioEmitterComponent, TransformComponent>();

    for (auto e : view)
    {
        auto [emitter, transform] = view.get<AudioEmitterComponent, TransformComponent>(e);
        AudioManager& am = AudioManager::Get();
        // recreate if asset or spatialization changes
        if (emitter.recreate)
        {
            am.Destroy(&emitter);
            am.Create(&emitter, e);
            emitter.recreate = false;
        }

        if (emitter.spatial)
        {
            am.SetPosition(&emitter, transform.GetWorldPosition());
        }
    }

    
    
}

void AudioSystem::BindCallbacks()
{
	m_Scene.GetRegistry().on_construct<AudioEmitterComponent>().connect<&AudioSystem::OnAudioEmitterCostructed>(this);
	m_Scene.GetRegistry().on_destroy<AudioEmitterComponent>().connect<&AudioSystem::OnAudioEmitterDestructed>(this);
}

void AudioSystem::OnAudioEmitterCostructed(entt::registry& reg, entt::entity e)
{
	auto& emitter = reg.get<AudioEmitterComponent>(e);

	AudioManager::Get().Create(&emitter, e);
}


void AudioSystem::OnAudioEmitterDestructed(entt::registry& reg, entt::entity e)
{
	auto& emitter = reg.get<AudioEmitterComponent>(e);

	AudioManager::Get().Destroy(&emitter);
}
