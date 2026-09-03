#include "rvelapch.h"
#include "AudioSystem.h"
#include "Scene/Scene.h"
#include "AudioManager.h"

using namespace rv;

void AudioSystem::OnStart()
{
    auto& reg = m_Scene.GetRegistry();
    auto view = reg.view<AudioEmitterComponent, TransformComponent>();
    AudioManager& am = AudioManager::Get();

    for (auto e : view)
    {
        auto [emitter, transform] = view.get<AudioEmitterComponent, TransformComponent>(e);

        if (emitter.playOnCreate)
            am.Play(&emitter);
    }
}

void AudioSystem::Update()
{
    auto& reg = m_Scene.GetRegistry();
    auto view = reg.view<AudioEmitterComponent, TransformComponent>();
    AudioManager& am = AudioManager::Get();

    for (auto e : view)
    {
        auto [emitter, transform] = view.get<AudioEmitterComponent, TransformComponent>(e);
        am.SyncState(&emitter);

        PlaybackState state = am.GetPlaybackState(&emitter);

        if (state == PlaybackState::Finished)
        {
            m_EventQueue.push_back({ e, Audio::EventType::FINISHED, emitter.audioClipUUID });
            am.DestroyInstance(&emitter);
        }
        else if (state == PlaybackState::Stopped)
        {
            am.DestroyInstance(&emitter);
        }

        if (emitter.spatial)
        {
            glm::vec3 worldPos = transform.GetWorldPosition();
            am.SetPosition(&emitter, worldPos);
            if (emitter.doppler)
            {
                if (emitter.prevPosValid)
                {
                    glm::vec3 vel = (worldPos - emitter.prevWorldPos) / Time::GetDeltaTime();
                    am.SetVelocity(&emitter, vel);
                }
                emitter.prevWorldPos = worldPos;
                emitter.prevPosValid = true;
            }
        }
    }
}

void AudioSystem::BindCallbacks()
{
	m_Scene.GetRegistry().on_destroy<AudioEmitterComponent>().connect<&AudioSystem::OnAudioEmitterDestructed>(this);
}


void AudioSystem::OnAudioEmitterDestructed(entt::registry& reg, entt::entity e)
{
	auto& emitter = reg.get<AudioEmitterComponent>(e);

	AudioManager::Get().DestroyInstance(&emitter);
}
