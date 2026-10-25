#include "rvelapch.h"
#include "AudioBus.h"

namespace rv {

static void AudioBusEffectNodeProcess(
    ma_node* pNode,
    const float** ppFramesIn,
    ma_uint32* pFrameCountIn,
    float** ppFramesOut,
    ma_uint32* pFrameCountOut)
{
    auto* effectNode = reinterpret_cast<BusEffectNode*>(pNode);
    if (!effectNode || !effectNode->bus || !ppFramesIn || !ppFramesOut || !ppFramesIn[0] || !ppFramesOut[0])
    {
        *pFrameCountIn = 0;
        *pFrameCountOut = 0;
        return;
    }

    ma_uint32 frameCount = std::min(*pFrameCountIn, *pFrameCountOut);
    if (frameCount == 0)
    {
        *pFrameCountIn = 0;
        *pFrameCountOut = 0;
        return;
    }

    ma_uint32 channels = ma_node_get_output_channels(pNode, 0);
    ma_uint32 sampleRate = ma_engine_get_sample_rate(effectNode->bus->GetEngine());

    effectNode->bus->ProcessEffects(ppFramesIn[0], ppFramesOut[0], frameCount, channels, sampleRate);

    *pFrameCountIn = frameCount;
    *pFrameCountOut = frameCount;
}

static ma_node_vtable s_AudioBusEffectNodeVTable =
{
    AudioBusEffectNodeProcess,
    nullptr,
    1,
    1,
    0
};

AudioBus::AudioBus(ma_engine* engine, ma_sound_group* parent)
    : m_Engine(engine)
{
    ma_result result = ma_sound_group_init(
        engine,
        0,
        nullptr,
        &m_Group
    );
    assert(result == MA_SUCCESS);

    ma_node_graph* nodeGraph = ma_engine_get_node_graph(engine);
    ma_uint32 channels = ma_engine_get_channels(engine);

    ma_node_config nodeConfig = ma_node_config_init();
    nodeConfig.vtable = &s_AudioBusEffectNodeVTable;
    nodeConfig.pInputChannels = &channels;
    nodeConfig.pOutputChannels = &channels;

    result = ma_node_init(nodeGraph, &nodeConfig, nullptr, &m_EffectNode.base);
    assert(result == MA_SUCCESS);
    m_EffectNode.bus = this;
    m_EffectNodeInitialized = true;

    result = ma_node_attach_output_bus(&m_Group, 0, &m_EffectNode.base, 0);
    assert(result == MA_SUCCESS);

    if (parent)
    {
        result = ma_node_attach_output_bus(&m_EffectNode.base, 0, parent, 0);
    }
    else
    {
        result = ma_node_attach_output_bus(&m_EffectNode.base, 0, ma_node_graph_get_endpoint(nodeGraph), 0);
    }
    assert(result == MA_SUCCESS);
}

AudioBus::~AudioBus()
{
    if (m_EffectNodeInitialized)
    {
        ma_node_uninit(&m_EffectNode.base, nullptr);
        m_EffectNodeInitialized = false;
    }
    ma_sound_group_uninit(&m_Group);
}

ma_sound_group* AudioBus::GetGroup()
{
    return &m_Group;
}

ma_node* AudioBus::GetOutputNode()
{
    return reinterpret_cast<ma_node*>(&m_EffectNode.base);
}

void AudioBus::AddEffect(std::shared_ptr<AudioEffect> effect)
{
    if (!effect) return;
    std::lock_guard<std::mutex> lock(m_EffectMutex);
    m_Effects.push_back(effect);
}

void AudioBus::RemoveEffect(size_t index)
{
    std::lock_guard<std::mutex> lock(m_EffectMutex);
    if (index < m_Effects.size())
        m_Effects.erase(m_Effects.begin() + index);
}

void AudioBus::MoveEffect(size_t fromIndex, size_t toIndex)
{
    std::lock_guard<std::mutex> lock(m_EffectMutex);
    if (fromIndex >= m_Effects.size() || toIndex >= m_Effects.size() || fromIndex == toIndex)
        return;

    auto elem = m_Effects[fromIndex];
    m_Effects.erase(m_Effects.begin() + fromIndex);
    m_Effects.insert(m_Effects.begin() + toIndex, elem);
}

json AudioBus::SerializeEffects() const
{
    std::lock_guard<std::mutex> lock(m_EffectMutex);
    json arr = json::array();
    for (const auto& fx : m_Effects)
    {
        if (fx)
            arr.push_back(fx->Serialize());
    }
    return arr;
}

void AudioBus::DeserializeEffects(const json& j)
{
    std::lock_guard<std::mutex> lock(m_EffectMutex);
    m_Effects.clear();

    if (!j.is_array()) return;

    for (const auto& fxJson : j)
    {
        if (!fxJson.contains("type")) continue;
        auto type = static_cast<AudioEffectType>(fxJson["type"].get<int>());
        auto fx = CreateAudioEffect(type);
        if (fx)
        {
            fx->Deserialize(fxJson);
            m_Effects.push_back(fx);
        }
    }
}

void AudioBus::ProcessEffects(const float* in, float* out, uint32_t frameCount, uint32_t channels, uint32_t sampleRate)
{
    std::unique_lock<std::mutex> lock(m_EffectMutex, std::try_to_lock);
    if (!lock.owns_lock() || m_Effects.empty())
    {
        std::memcpy(out, in, frameCount * channels * sizeof(float));
        return;
    }

    bool anyEnabled = false;
    for (const auto& fx : m_Effects)
    {
        if (fx && fx->enabled)
        {
            anyEnabled = true;
            break;
        }
    }

    if (!anyEnabled)
    {
        std::memcpy(out, in, frameCount * channels * sizeof(float));
        return;
    }

    size_t sampleCount = static_cast<size_t>(frameCount) * channels;
    if (m_IntermediateBuffer.size() < sampleCount)
        m_IntermediateBuffer.resize(sampleCount);

    float* bufA = m_IntermediateBuffer.data();
    float* bufB = out;

    bool first = true;
    for (size_t i = 0; i < m_Effects.size(); ++i)
    {
        auto& fx = m_Effects[i];
        if (!fx || !fx->enabled) continue;

        const float* src = first ? in : bufA;
        float* dst = bufB;

        fx->Process(src, dst, frameCount, channels, sampleRate);

        first = false;
        std::swap(bufA, bufB);
    }

    if (bufA != out)
    {
        std::memcpy(out, m_IntermediateBuffer.data(), sampleCount * sizeof(float));
    }
}

}
