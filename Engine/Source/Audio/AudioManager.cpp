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
    assert(result == MA_SUCCESS);

    CreateMasterBus();

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

const AudioInstance* AudioManager::GetInstance(AudioEmitterComponent* comp) const
{
    if (!comp || comp->instanceID == UINT32_MAX) return nullptr;
    auto it = m_Instances.find(comp->instanceID);
    return (it != m_Instances.end()) ? &it->second : nullptr;
}

ma_attenuation_model AudioManager::ToMA(AttenuationModel model)
{
    switch (model)
    {
    case AttenuationModel::None: return ma_attenuation_model_none;
    case AttenuationModel::Inverse: return ma_attenuation_model_inverse;
    case AttenuationModel::Linear: return ma_attenuation_model_linear;
    case AttenuationModel::Exponential: return ma_attenuation_model_exponential;
    default: return ma_attenuation_model_inverse;
    }
}

void AudioManager::CreateMasterBus()
{
    auto master = std::make_unique<AudioBus>(&m_Engine, nullptr);
    master->SetID(m_NextBusID++);
    master->SetName("Master");
    m_MasterBus = master.get();
    m_Busses.push_back(std::move(master));
}

void AudioManager::Create(AudioEmitterComponent* comp, entt::entity entity)
{
    if (!comp || comp->path.empty()) return;
    Destroy(comp);

    uint32_t id = m_NextID++;

    AudioInstance& instance = m_Instances[id];
    instance.entity = entity;

    AudioBus* bus = GetBus(comp->busID);
    if (!bus)
        bus = m_MasterBus;
    instance.busID = bus->GetID();


    ma_result result = ma_sound_init_from_file(
        &m_Engine,
        comp->path.c_str(),
        0,
        bus->GetGroup(),
        nullptr,
        &instance.sound
    );

    if (result != MA_SUCCESS)
    {
        m_Instances.erase(id);
        return;
    }
    comp->instanceID = id;

    SetVolume(comp, comp->volume);
    SetPitch(comp, comp->pitch);
    SetLoop(comp, comp->loop);
    SetSpatial(comp, comp->spatial);
    SetMinDistance(comp, comp->minDistance);
    SetMaxDistance(comp, comp->maxDistance);
    SetRolloff(comp, comp->rolloff);
    SetDopplerFactor(comp, comp->dopplerFactor);
    if (comp->playOnCreate)
    {
        Play(comp);
    }


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

void AudioManager::Pause(AudioEmitterComponent* comp)
{
    if (auto* inst = GetInstance(comp))
        ma_sound_stop(&inst->sound);
}

void AudioManager::Resume(AudioEmitterComponent* comp)
{
    if (auto* inst = GetInstance(comp))
        if (!ma_sound_at_end(&inst->sound))
            ma_sound_start(&inst->sound);
}

bool AudioManager::IsPlaying(AudioEmitterComponent* comp) const
{
    if (const auto* inst = GetInstance(comp))
        return ma_sound_is_playing(&inst->sound) == MA_TRUE;
    return false;
}

bool AudioManager::IsPaused(AudioEmitterComponent* comp) const
{
    if (const auto* inst = GetInstance(comp))
    {
        bool playing = ma_sound_is_playing(&inst->sound) == MA_TRUE;
        bool finished = ma_sound_at_end(&inst->sound) == MA_TRUE;
        return !playing && !finished;
    }
    return false;
}

void AudioManager::Seek(AudioEmitterComponent* comp, float seconds)
{
    if (auto* inst = GetInstance(comp))
    {
        ma_uint32 sampleRate = 0;
        ma_sound_get_data_format(&inst->sound, nullptr, nullptr, &sampleRate, nullptr, 0);
        if (sampleRate > 0)
        {
            ma_uint64 frame = static_cast<ma_uint64>(seconds * sampleRate);
            ma_sound_seek_to_pcm_frame(&inst->sound, frame);
        }
    }
}

float AudioManager::GetPlaybackPosition(AudioEmitterComponent* comp) const
{
    if (const auto* inst = GetInstance(comp))
    {
        float cursor = 0.0f;
        ma_sound_get_cursor_in_seconds(&inst->sound, &cursor);
        return cursor;
    }
    return 0.0f;
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

void AudioManager::SetSpatial(AudioEmitterComponent* comp, bool spatial)
{
    if (!comp) return;
    comp->spatial = spatial;

    if (auto* inst = GetInstance(comp))
        ma_sound_set_spatialization_enabled(&inst->sound, spatial ? MA_TRUE : MA_FALSE);

}

void AudioManager::SetPosition(AudioEmitterComponent* comp, const glm::vec3& pos)
{
    if (auto* inst = GetInstance(comp))
        ma_sound_set_position(&inst->sound, pos.x, pos.y, pos.z);
}

void AudioManager::SetVelocity(AudioEmitterComponent* comp, const glm::vec3& vel)
{
    if (!comp) return;
    if (auto* inst = GetInstance(comp))
        ma_sound_set_velocity(&inst->sound, vel.x, vel.y, vel.z);
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

void AudioManager::SetListenerVelocity(const glm::vec3& vel)
{
    ma_engine_listener_set_velocity(&m_Engine, 0, vel.x, vel.y, vel.z);
}



void AudioManager::SetAttenuationModel(AudioEmitterComponent* comp, AttenuationModel model)
{
    if (!comp) return;
    comp->attenuationModel = model;
    if (auto* inst = GetInstance(comp))
        ma_sound_set_attenuation_model(&inst->sound, ToMA(model));
}

void AudioManager::SetMinDistance(AudioEmitterComponent* comp, float minDist)
{
    if (!comp) return;
    comp->minDistance = minDist;
    if (auto* inst = GetInstance(comp))
        ma_sound_set_min_distance(&inst->sound, minDist);
}

void AudioManager::SetMaxDistance(AudioEmitterComponent* comp, float maxDist)
{
    if (!comp) return;
    comp->maxDistance = maxDist;
    if (auto* inst = GetInstance(comp))
        ma_sound_set_max_distance(&inst->sound, maxDist);
}

void AudioManager::SetRolloff(AudioEmitterComponent* comp, float rolloff)
{
    if (!comp) return;
    comp->rolloff = rolloff;
    if (auto* inst = GetInstance(comp))
        ma_sound_set_rolloff(&inst->sound, rolloff);
}

void AudioManager::SetDopplerFactor(AudioEmitterComponent* comp, float factor)
{
    if (!comp) return;
    comp->dopplerFactor = factor;
    if (auto* inst = GetInstance(comp))
        ma_sound_set_doppler_factor(&inst->sound, comp->doppler ? factor : 0.0f);
}

AudioBus* AudioManager::CreateBus()
{
    auto bus = std::make_unique<AudioBus>(&m_Engine, nullptr);

    bus->SetID(m_NextBusID);
    bus->SetName("Bus " + std::to_string(m_NextBusID));
    bus->SetParentBusID(m_MasterBus->GetID());

    AudioBus* result = bus.get();

    m_Busses.push_back(std::move(bus));

    m_NextBusID++;

    return result;
}

bool AudioManager::SetParentBus(uint32_t busID, uint32_t parentBusID)
{
    AudioBus* bus = GetBus(busID);
    AudioBus* parent = GetBus(parentBusID);

    if (!bus || !parent)
        return false;

    if (bus == parent)
        return false;

    ma_node_detach_output_bus(
        bus->GetGroup(),
        0
    );

    ma_node_attach_output_bus(
        bus->GetGroup(),
        0,
        parent->GetGroup(),
        0
    );

    bus->SetParentBusID(parentBusID);

    return true;
}

void AudioManager::SetBus(AudioEmitterComponent* comp, uint32_t busID)
{
    if (!comp)
        return;

    AudioInstance* instance = GetInstance(comp);
    AudioBus* bus = GetBus(busID);

    if (!instance || !bus)
        return;

    ma_node_attach_output_bus(
        &instance->sound,
        0,
        bus->GetGroup(),
        0
    );

    comp->busID = busID;
    instance->busID = busID;
}

bool AudioManager::DestroyBus(uint32_t busID)
{
    auto it = std::find_if(
        m_Busses.begin(),
        m_Busses.end(),
        [busID](const std::unique_ptr<AudioBus>& bus)
        {
            return bus->GetID() == busID;
        }
    );

    if (it == m_Busses.end())
        return false;

    m_Busses.erase(it);
    return true;
}

AudioBus* AudioManager::GetBus(uint32_t id)
{
    auto it = std::find_if(
        m_Busses.begin(),
        m_Busses.end(),
        [id](const auto& bus)
        {
            return bus->GetID() == id;
        }
    );

    return it != m_Busses.end() ? it->get() : nullptr;
}

int AudioManager::GetBusID(const std::string& name)
{
    auto it = std::find_if(
        m_Busses.begin(),
        m_Busses.end(),
        [&name](const auto& bus)
        {
            return bus->GetName() == name;
        }
    );

    return it != m_Busses.end() ? it->get()->GetID() : -1;
}

void AudioManager::SetBusVolume(uint32_t id, float volume)
{
    AudioBus* bus = GetBus(id);
    if (!bus) return;

    ma_sound_group_set_volume(bus->GetGroup(), volume);
}

void rv::AudioManager::Update(Camera* camera)
{
    if (camera)
    {
        AudioManager::Get().SetListenerPosition(camera->Position);
        AudioManager::Get().SetListenerDirection(camera->Front, camera->Up);
        AudioManager::Get().SetListenerVelocity(camera->Velocity);
    }
}
