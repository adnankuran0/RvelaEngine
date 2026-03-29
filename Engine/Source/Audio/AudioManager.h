#pragma once
#include "miniaudio/miniaudio.h"
#include "Scene/Components/AudioEmitterComponent.h"
#include "Audio/AudioInstance.h"
#include <entt/entt.h>


namespace rv {

class Camera;

class AudioManager
{
public:
    AudioManager();
    ~AudioManager();

    inline static void Init() { Get(); }
    static AudioManager& Get() 
    { 
        static AudioManager instance; 
        return instance; 
    }

    void Create(AudioEmitterComponent* comp, entt::entity entity);
    void Destroy(AudioEmitterComponent* comp);

    void Play(AudioEmitterComponent* comp);
    void Stop(AudioEmitterComponent* comp);
    void Pause(AudioEmitterComponent* comp);
    void Resume(AudioEmitterComponent* comp);

    bool IsPlaying(AudioEmitterComponent* comp) const;
    bool IsPaused(AudioEmitterComponent* comp) const;

    void Seek(AudioEmitterComponent* comp, float seconds);
    float GetPlaybackPosition(AudioEmitterComponent* comp) const;

    void SetVolume(AudioEmitterComponent* comp, float volume);
    void SetPitch(AudioEmitterComponent* comp, float pitch);
    void SetLoop(AudioEmitterComponent* comp, bool loop);

    void SetPosition(AudioEmitterComponent* comp, const glm::vec3& pos);
    void SetVelocity(AudioEmitterComponent* comp, const glm::vec3& vel);

    void SetAttenuationModel(AudioEmitterComponent* comp, AttenuationModel model);
    void SetMinDistance(AudioEmitterComponent* comp, float minDist);
    void SetMaxDistance(AudioEmitterComponent* comp, float maxDist);
    void SetRolloff(AudioEmitterComponent* comp, float rolloff);

    void SetDopplerFactor(AudioEmitterComponent* comp, float factor);

    void SetListenerPosition(const glm::vec3& pos);
    void SetListenerDirection(const glm::vec3& forward, const glm::vec3& up);
    void SetListenerVelocity(const glm::vec3& vel);

    static void Update(Camera* camera);

private:
    AudioInstance* GetInstance(AudioEmitterComponent* comp);
    const AudioInstance* GetInstance(AudioEmitterComponent* comp) const;
    static ma_attenuation_model ToMA(AttenuationModel model);
private:
    ma_engine m_Engine;
    
    std::unordered_map<uint32_t, AudioInstance> m_Instances;
    uint32_t m_NextID = 0;
};

}