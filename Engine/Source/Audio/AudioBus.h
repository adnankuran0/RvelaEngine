#pragma once
#include "miniaudio/miniaudio.h"
#include "Audio/AudioEffects.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace rv {

class AudioBus;

struct BusEffectNode
{
    ma_node_base base;
    AudioBus* bus = nullptr;
};

class AudioBus
{
public:
    AudioBus(ma_engine* engine, ma_sound_group* parent);
    ~AudioBus();

    const std::string& GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }

    unsigned int GetID() const { return m_ID; }
    void SetID(unsigned int id) { m_ID = id; }

    unsigned int GetParentBusID() const { return m_ParentBus; }
    void SetParentBusID(unsigned int parentBusID) { m_ParentBus = parentBusID; }

    ma_sound_group* GetGroup();
    ma_node* GetOutputNode();
    ma_engine* GetEngine() const { return m_Engine; }

    std::vector<std::shared_ptr<AudioEffect>>& GetEffects() { return m_Effects; }
    const std::vector<std::shared_ptr<AudioEffect>>& GetEffects() const { return m_Effects; }

    void AddEffect(std::shared_ptr<AudioEffect> effect);
    void RemoveEffect(size_t index);
    void MoveEffect(size_t fromIndex, size_t toIndex);

    json SerializeEffects() const;
    void DeserializeEffects(const json& j);

    void ProcessEffects(const float* in, float* out, uint32_t frameCount, uint32_t channels, uint32_t sampleRate);

private:
    unsigned int m_ID = 0;
    unsigned int m_ParentBus = 0; // master bus by default
    std::string m_Name;
    ma_sound_group m_Group{};
    BusEffectNode m_EffectNode{};
    bool m_EffectNodeInitialized = false;
    ma_engine* m_Engine = nullptr;

    std::vector<std::shared_ptr<AudioEffect>> m_Effects;
    mutable std::mutex m_EffectMutex;
    std::vector<float> m_IntermediateBuffer;
};

} // namespace rv
