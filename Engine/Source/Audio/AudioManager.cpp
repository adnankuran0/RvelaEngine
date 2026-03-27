#include "rvelapch.h"
#define MINIAUDIO_IMPLEMENTATION
#include "AudioManager.h"
#include <Renderer/Camera.h>


using namespace rv;

AudioManager::AudioManager()
{
    ma_engine_config cfg = ma_engine_config_init();
    cfg.listenerCount = 1;
    ma_engine_init(&cfg, &m_Engine);
}

AudioManager::~AudioManager()
{
    for (auto& [id, inst] : m_Instances)
        ma_sound_uninit(&inst.sound);
    ma_engine_uninit(&m_Engine);
}

AudioInstance* AudioManager::GetInstance(AudioEmitterComponent* comp)
{
    if (!comp || comp->instanceID == UINT32_MAX) return nullptr;

    auto it = m_Instances.find(comp->instanceID);
    if (it == m_Instances.end()) return nullptr;

    return &it->second;
}

void AudioManager::Create(AudioEmitterComponent* comp, entt::entity entity)
{
    if (!comp || comp->path.empty()) return;
    Destroy(comp);

    uint32_t id = m_NextID++;

    AudioInstance& instance = m_Instances[id];
    instance.entity = entity;

    ma_result result = ma_sound_init_from_file(
        &m_Engine,
        comp->path.c_str(),
        0, nullptr, nullptr,
        &instance.sound
    );

    if (result != MA_SUCCESS)
    {
        m_Instances.erase(id);
        return;
    }

    comp->instanceID = id;
    SetLoop(comp, comp->loop);
}


void AudioManager::Destroy(AudioEmitterComponent* comp)
{
    if (!comp || comp->instanceID == UINT32_MAX) return;

    auto it = m_Instances.find(comp->instanceID);
    if (it != m_Instances.end())
    {
        ma_sound_uninit(&it->second.sound);
        m_Instances.erase(it);
    }

    comp->instanceID = UINT32_MAX;
}

void AudioManager::Play(AudioEmitterComponent* comp)
{
    if (auto* inst = GetInstance(comp))
        ma_sound_start(&inst->sound);
}

void AudioManager::Stop(AudioEmitterComponent* comp)
{
    if (auto* inst = GetInstance(comp))
        ma_sound_stop(&inst->sound);
}

void AudioManager::SetVolume(AudioEmitterComponent* comp, float volume)
{
    if (!comp) return;
    comp->volume = volume;

    if (auto* inst = GetInstance(comp))
        ma_sound_set_volume(&inst->sound, volume);
}

void AudioManager::SetPitch(AudioEmitterComponent* comp, float pitch)
{
    if (!comp) return;
    comp->pitch = pitch;

    if (auto* inst = GetInstance(comp))
        ma_sound_set_pitch(&inst->sound, pitch);
}

void AudioManager::SetLoop(AudioEmitterComponent* comp, bool loop)
{
    if (!comp) return;
    comp->loop = loop;

    if (auto* inst = GetInstance(comp))
        ma_sound_set_looping(&inst->sound, loop ? MA_TRUE : MA_FALSE);
}

void AudioManager::SetPosition(AudioEmitterComponent* comp, const glm::vec3& pos)
{
    if (auto* inst = GetInstance(comp))
        ma_sound_set_position(&inst->sound, pos.x, pos.y, pos.z);
}

void AudioManager::SetListenerPosition(const glm::vec3& pos)
{
    ma_engine_listener_set_position(&m_Engine, 0, pos.x, pos.y, pos.z);
}

void AudioManager::SetListenerDirection(const glm::vec3& forward, const glm::vec3& up)
{
    ma_engine_listener_set_direction(&m_Engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&m_Engine, 0, up.x, up.y, up.z);
}

void rv::AudioManager::Update(Camera* camera)
{
    if (camera)
    {
        AudioManager::Get().SetListenerPosition(camera->Position);
        AudioManager::Get().SetListenerDirection(camera->Front, camera->Up);
    }
}
