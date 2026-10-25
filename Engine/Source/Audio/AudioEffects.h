#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include "json.hpp"

namespace rv {

using json = nlohmann::json;

enum class AudioEffectType
{
    Reverb = 0,
    Limiter,
    Distortion,
    Delay,
    Filter,
    Equalizer
};

inline const char* AudioEffectTypeToString(AudioEffectType type)
{
    switch (type)
    {
    case AudioEffectType::Reverb:     return "Reverb";
    case AudioEffectType::Limiter:    return "Limiter";
    case AudioEffectType::Distortion: return "Distortion";
    case AudioEffectType::Delay:      return "Delay";
    case AudioEffectType::Filter:     return "Filter";
    case AudioEffectType::Equalizer:  return "Equalizer";
    default:                          return "Unknown";
    }
}

enum class DistortionType
{
    SoftClip = 0,
    HardClip,
    Tanh,
    Foldback,
    Bitcrush
};

inline const char* DistortionTypeToString(DistortionType type)
{
    switch (type)
    {
    case DistortionType::SoftClip: return "Soft Clip";
    case DistortionType::HardClip: return "Hard Clip";
    case DistortionType::Tanh:     return "Tape / Tanh";
    case DistortionType::Foldback: return "Foldback";
    case DistortionType::Bitcrush: return "Bitcrush";
    default:                       return "Unknown";
    }
}

enum class FilterType
{
    LowPass = 0,
    HighPass,
    BandPass,
    Notch
};

inline const char* FilterTypeToString(FilterType type)
{
    switch (type)
    {
    case FilterType::LowPass:  return "Low Pass";
    case FilterType::HighPass: return "High Pass";
    case FilterType::BandPass: return "Band Pass";
    case FilterType::Notch:    return "Notch";
    default:                   return "Unknown";
    }
}

class AudioEffect
{
public:
    virtual ~AudioEffect() = default;

    virtual AudioEffectType GetType() const = 0;
    virtual const char* GetTypeName() const { return AudioEffectTypeToString(GetType()); }

    virtual void Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate) = 0;
    virtual void Reset() {}

    virtual json Serialize() const;
    virtual void Deserialize(const json& j);

    bool enabled = true;
};

class AudioReverbEffect : public AudioEffect
{
public:
    AudioReverbEffect();

    AudioEffectType GetType() const override { return AudioEffectType::Reverb; }
    void Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate) override;
    void Reset() override;

    json Serialize() const override;
    void Deserialize(const json& j) override;

    float roomSize = 0.5f;
    float damping = 0.2f;
    float width = 1.0f;
    float wet = 0.33f;
    float dry = 1.0f;
    bool freeze = false;

private:
    struct CombFilter
    {
        std::vector<float> buffer;
        size_t index = 0;
        float store = 0.0f;

        void Init(size_t size);
        void Clear();
        float Process(float input, float feedback, float damp);
    };

    struct AllpassFilter
    {
        std::vector<float> buffer;
        size_t index = 0;

        void Init(size_t size);
        void Clear();
        float Process(float input);
    };

    void UpdateSampleRate(uint32_t sampleRate);

    static constexpr int kNumCombs = 8;
    static constexpr int kNumAllpasses = 4;

    CombFilter m_CombsL[kNumCombs];
    CombFilter m_CombsR[kNumCombs];
    AllpassFilter m_AllpassesL[kNumAllpasses];
    AllpassFilter m_AllpassesR[kNumAllpasses];

    uint32_t m_CurrentSampleRate = 0;
};

class AudioLimiterEffect : public AudioEffect
{
public:
    AudioLimiterEffect() = default;

    AudioEffectType GetType() const override { return AudioEffectType::Limiter; }
    void Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate) override;
    void Reset() override;

    json Serialize() const override;
    void Deserialize(const json& j) override;

    float thresholdDb = -1.0f;
    float ceilingDb = -0.1f;
    float releaseMs = 50.0f;
    float makeupGainDb = 0.0f;

    float currentGainReduction = 1.0f;

private:
    float m_Envelope = 0.0f;
};

class AudioDistortionEffect : public AudioEffect
{
public:
    AudioDistortionEffect() = default;

    AudioEffectType GetType() const override { return AudioEffectType::Distortion; }
    void Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate) override;
    void Reset() override;

    json Serialize() const override;
    void Deserialize(const json& j) override;

    DistortionType type = DistortionType::Tanh;
    float drive = 2.0f; // 1.0 to 50.0
    float mix = 1.0f;
    float tone = 0.8f;
    float outputGain = 1.0f; // 0.0 to 2.0
    int bits = 8; // 2 to 16 for bitcrush

private:
    float m_FilterL = 0.0f;
    float m_FilterR = 0.0f;
};

class AudioDelayEffect : public AudioEffect
{
public:
    AudioDelayEffect();

    AudioEffectType GetType() const override { return AudioEffectType::Delay; }
    void Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate) override;
    void Reset() override;

    json Serialize() const override;
    void Deserialize(const json& j) override;

    float delayTime = 0.3f; // 0.01s to 2.0s
    float feedback = 0.4f; // 0.0 to 0.95
    float mix = 0.35f;
    bool pingPong = false;

private:
    void EnsureBufferSize(uint32_t sampleRate);

    std::vector<float> m_BufferL;
    std::vector<float> m_BufferR;
    size_t m_WriteIndex = 0;
    uint32_t m_CurrentSampleRate = 0;
    float m_FilterL = 0.0f;
    float m_FilterR = 0.0f;
};

class AudioFilterEffect : public AudioEffect
{
public:
    AudioFilterEffect() = default;

    AudioEffectType GetType() const override { return AudioEffectType::Filter; }
    void Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate) override;
    void Reset() override;

    json Serialize() const override;
    void Deserialize(const json& j) override;

    FilterType type = FilterType::LowPass;
    float cutoffFrequency = 1000.0f;
    float resonance = 0.707f; // 0.1 to 10.0

private:
    float m_LpL = 0.0f, m_BpL = 0.0f;
    float m_LpR = 0.0f, m_BpR = 0.0f;
};

class AudioEqualizerEffect : public AudioEffect
{
public:
    AudioEqualizerEffect() = default;

    AudioEffectType GetType() const override { return AudioEffectType::Equalizer; }
    void Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate) override;
    void Reset() override;

    json Serialize() const override;
    void Deserialize(const json& j) override;

    // -24.0 to +24.0 dB
    float lowGainDb = 0.0f;   
    float midGainDb = 0.0f;
    float highGainDb = 0.0f;

    float lowCutoff = 250.0f;
    float highCutoff = 4000.0f;

private:
    float m_LowL = 0.0f, m_MidL = 0.0f;
    float m_LowR = 0.0f, m_MidR = 0.0f;
};

std::shared_ptr<AudioEffect> CreateAudioEffect(AudioEffectType type);

}
