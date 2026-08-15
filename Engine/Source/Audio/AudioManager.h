#pragma once
#include "miniaudio/miniaudio.h"
#include "Scene/Components/AudioEmitterComponent.h"
#include "Audio/AudioInstance.h"
#include "Audio/AudioBus.h"
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

    void CreateInstance(AudioEmitterComponent* comp);
    void DestroyInstance(AudioEmitterComponent* comp);

    void SetClip(AudioEmitterComponent* comp, Ref<AudioClipAsset> clip);

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
    void SetSpatial(AudioEmitterComponent* comp, bool spatial);

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

    AudioBus* CreateBus();
    bool DestroyBus(uint32_t busID);
    AudioBus* GetBus(uint32_t id);
    int GetBusID(const std::string& name);
    void SetBusVolume(uint32_t busID, float volume);
    float GetBusVolume(uint32_t busID);
    bool SetParentBus(uint32_t busID, uint32_t parentBusID);
    void SetBus(AudioEmitterComponent* comp, uint32_t busID);
    std::vector<std::unique_ptr<AudioBus>>& GetBusses() { return m_Busses; }

    json SaveBusLayout();
    void LoadBusLayout(const std::string& jsonStr);

    static void Update(Camera* camera);

private:
    AudioInstance* GetInstance(AudioEmitterComponent* comp);
    const AudioInstance* GetInstance(AudioEmitterComponent* comp) const;
    static ma_attenuation_model ToMA(AttenuationModel model);
    void CreateMasterBus();
private:
    ma_engine m_Engine;
    
    uint32_t m_NextInstanceID = 0;
    std::unordered_map<uint32_t, AudioInstance> m_Instances;

    std::vector<std::unique_ptr<AudioBus>> m_Busses;
    AudioBus* m_MasterBus = nullptr;
    uint32_t m_NextBusID = 1; // 0 is assigned to master bus
};

}