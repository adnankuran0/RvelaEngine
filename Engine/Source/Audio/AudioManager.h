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

    static AudioManager& Get() { static AudioManager instance; return instance; }

    void Create(AudioEmitterComponent* comp, entt::entity entity);
    void Destroy(AudioEmitterComponent* comp);

    void Play(AudioEmitterComponent* comp);
    void Stop(AudioEmitterComponent* comp);

    void SetVolume(AudioEmitterComponent* comp, float volume);
    void SetPitch(AudioEmitterComponent* comp, float pitch);
    void SetLoop(AudioEmitterComponent* comp, bool loop);
    void SetPosition(AudioEmitterComponent* comp, const glm::vec3& pos);
    void SetListenerPosition(const glm::vec3& pos);
    void SetListenerDirection(const glm::vec3& forward, const glm::vec3& up);

    static void Update(Camera* camera);

    ma_engine& GetEngine() { return m_Engine; }

private:
    AudioInstance* GetInstance(AudioEmitterComponent* comp);

private:
    ma_engine m_Engine;
    std::unordered_map<uint32_t, AudioInstance> m_Instances;
    uint32_t m_NextID = 0;
};

}