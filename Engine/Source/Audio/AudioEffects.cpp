#include "rvelapch.h"
#include "AudioEffects.h"

namespace rv {

static constexpr float kPi = 3.14159265358979323846f;

inline float DenormalGuard(float val)
{
    return (std::abs(val) < 1e-15f) ? 0.0f : val;
}

json AudioEffect::Serialize() const
{
    json j;
    j["type"] = static_cast<int>(GetType());
    j["enabled"] = enabled;
    return j;
}

void AudioEffect::Deserialize(const json& j)
{
    if (j.contains("enabled"))
        enabled = j["enabled"].get<bool>();
}

static constexpr int kCombLengths44k[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
static constexpr int kAllpassLengths44k[4] = { 556, 441, 341, 225 };
static constexpr int kStereoSpread = 23;

AudioReverbEffect::AudioReverbEffect()
{
    UpdateSampleRate(44100);
}

void AudioReverbEffect::CombFilter::Init(size_t size)
{
    buffer.assign(size, 0.0f);
    index = 0;
    store = 0.0f;
}

void AudioReverbEffect::CombFilter::Clear()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    index = 0;
    store = 0.0f;
}

float AudioReverbEffect::CombFilter::Process(float input, float feedback, float damp)
{
    if (buffer.empty()) return input;

    float output = buffer[index];
    output = DenormalGuard(output);

    store = (output * (1.0f - damp)) + (store * damp);
    store = DenormalGuard(store);

    buffer[index] = input + (store * feedback);
    if (++index >= buffer.size())
        index = 0;

    return output;
}

void AudioReverbEffect::AllpassFilter::Init(size_t size)
{
    buffer.assign(size, 0.0f);
    index = 0;
}

void AudioReverbEffect::AllpassFilter::Clear()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    index = 0;
}

float AudioReverbEffect::AllpassFilter::Process(float input)
{
    if (buffer.empty()) return input;

    float bufOut = buffer[index];
    bufOut = DenormalGuard(bufOut);

    float output = -input + bufOut;
    buffer[index] = input + (bufOut * 0.5f);

    if (++index >= buffer.size())
        index = 0;

    return output;
}

void AudioReverbEffect::UpdateSampleRate(uint32_t sampleRate)
{
    if (sampleRate == 0) sampleRate = 44100;
    if (m_CurrentSampleRate == sampleRate) return;

    m_CurrentSampleRate = sampleRate;
    float scale = static_cast<float>(sampleRate) / 44100.0f;

    for (int i = 0; i < kNumCombs; ++i)
    {
        size_t lenL = std::max<size_t>(16, static_cast<size_t>(std::round(kCombLengths44k[i] * scale)));
        size_t lenR = std::max<size_t>(16, static_cast<size_t>(std::round((kCombLengths44k[i] + kStereoSpread) * scale)));
        m_CombsL[i].Init(lenL);
        m_CombsR[i].Init(lenR);
    }

    for (int i = 0; i < kNumAllpasses; ++i)
    {
        size_t lenL = std::max<size_t>(8, static_cast<size_t>(std::round(kAllpassLengths44k[i] * scale)));
        size_t lenR = std::max<size_t>(8, static_cast<size_t>(std::round((kAllpassLengths44k[i] + kStereoSpread) * scale)));
        m_AllpassesL[i].Init(lenL);
        m_AllpassesR[i].Init(lenR);
    }
}

void AudioReverbEffect::Reset()
{
    for (int i = 0; i < kNumCombs; ++i)
    {
        m_CombsL[i].Clear();
        m_CombsR[i].Clear();
    }
    for (int i = 0; i < kNumAllpasses; ++i)
    {
        m_AllpassesL[i].Clear();
        m_AllpassesR[i].Clear();
    }
}

void AudioReverbEffect::Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate)
{
    if (!enabled || frameCount == 0)
    {
        std::memcpy(output, input, frameCount * channels * sizeof(float));
        return;
    }

    UpdateSampleRate(sampleRate);

    float feedback = freeze ? 1.0f : (roomSize * 0.28f + 0.7f);
    float damp = freeze ? 0.0f : (damping * 0.4f);
    float gain = 0.015f;

    float wet1 = wet * (width * 0.5f + 0.5f);
    float wet2 = wet * ((1.0f - width) * 0.5f);

    for (uint32_t f = 0; f < frameCount; ++f)
    {
        float inL = input[f * channels];
        float inR = (channels > 1) ? input[f * channels + 1] : inL;
        float inMono = (inL + inR) * 0.5f * gain;

        float outL = 0.0f;
        float outR = 0.0f;

        for (int i = 0; i < kNumCombs; ++i)
        {
            outL += m_CombsL[i].Process(inMono, feedback, damp);
            outR += m_CombsR[i].Process(inMono, feedback, damp);
        }

        for (int i = 0; i < kNumAllpasses; ++i)
        {
            outL = m_AllpassesL[i].Process(outL);
            outR = m_AllpassesR[i].Process(outR);
        }

        float wetOutL = outL * wet1 + outR * wet2;
        float wetOutR = outR * wet1 + outL * wet2;

        output[f * channels] = inL * dry + wetOutL;
        if (channels > 1)
            output[f * channels + 1] = inR * dry + wetOutR;

        for (uint32_t c = 2; c < channels; ++c)
            output[f * channels + c] = input[f * channels + c];
    }
}

json AudioReverbEffect::Serialize() const
{
    json j = AudioEffect::Serialize();
    j["roomSize"] = roomSize;
    j["damping"] = damping;
    j["width"] = width;
    j["wet"] = wet;
    j["dry"] = dry;
    j["freeze"] = freeze;
    return j;
}

void AudioReverbEffect::Deserialize(const json& j)
{
    AudioEffect::Deserialize(j);
    if (j.contains("roomSize")) roomSize = j["roomSize"].get<float>();
    if (j.contains("damping"))  damping  = j["damping"].get<float>();
    if (j.contains("width"))    width    = j["width"].get<float>();
    if (j.contains("wet"))      wet      = j["wet"].get<float>();
    if (j.contains("dry"))      dry      = j["dry"].get<float>();
    if (j.contains("freeze"))   freeze   = j["freeze"].get<bool>();
}

void AudioLimiterEffect::Reset()
{
    m_Envelope = 0.0f;
    currentGainReduction = 1.0f;
}

void AudioLimiterEffect::Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate)
{
    if (!enabled || frameCount == 0)
    {
        std::memcpy(output, input, frameCount * channels * sizeof(float));
        currentGainReduction = 1.0f;
        return;
    }

    if (sampleRate == 0) sampleRate = 44100;

    float attackCoeff = std::exp(-1.0f / (0.0005f * static_cast<float>(sampleRate)));
    float releaseCoeff = std::exp(-1.0f / (std::max(releaseMs, 1.0f) * 0.001f * static_cast<float>(sampleRate)));

    float threshLin = std::pow(10.0f, thresholdDb / 20.0f);
    float ceilingLin = std::pow(10.0f, ceilingDb / 20.0f);
    float makeupLin = std::pow(10.0f, makeupGainDb / 20.0f);

    float minGain = 1.0f;

    for (uint32_t f = 0; f < frameCount; ++f)
    {
        float peak = 0.0f;
        for (uint32_t c = 0; c < channels; ++c)
        {
            float absSample = std::abs(input[f * channels + c]);
            if (absSample > peak) peak = absSample;
        }

        if (peak > m_Envelope)
            m_Envelope = attackCoeff * m_Envelope + (1.0f - attackCoeff) * peak;
        else
            m_Envelope = releaseCoeff * m_Envelope + (1.0f - releaseCoeff) * peak;

        m_Envelope = DenormalGuard(m_Envelope);

        float gain = 1.0f;
        if (m_Envelope > threshLin && m_Envelope > 1e-6f)
            gain = threshLin / m_Envelope;

        if (gain < minGain) minGain = gain;

        float effectiveGain = gain * makeupLin;
        for (uint32_t c = 0; c < channels; ++c)
        {
            float s = input[f * channels + c] * effectiveGain;
            output[f * channels + c] = std::clamp(s, -ceilingLin, ceilingLin);
        }
    }

    currentGainReduction = minGain;
}

json AudioLimiterEffect::Serialize() const
{
    json j = AudioEffect::Serialize();
    j["thresholdDb"] = thresholdDb;
    j["ceilingDb"] = ceilingDb;
    j["releaseMs"] = releaseMs;
    j["makeupGainDb"] = makeupGainDb;
    return j;
}

void AudioLimiterEffect::Deserialize(const json& j)
{
    AudioEffect::Deserialize(j);
    if (j.contains("thresholdDb"))  thresholdDb  = j["thresholdDb"].get<float>();
    if (j.contains("ceilingDb"))    ceilingDb    = j["ceilingDb"].get<float>();
    if (j.contains("releaseMs"))    releaseMs    = j["releaseMs"].get<float>();
    if (j.contains("makeupGainDb")) makeupGainDb = j["makeupGainDb"].get<float>();
}

static float SaturateSample(float x, DistortionType type, int bits)
{
    switch (type)
    {
    case DistortionType::SoftClip:
        if (x > 1.0f) return 2.0f / 3.0f;
        if (x < -1.0f) return -2.0f / 3.0f;
        return x - (x * x * x) / 3.0f;

    case DistortionType::HardClip:
        return std::clamp(x, -1.0f, 1.0f);

    case DistortionType::Tanh:
        return std::tanh(x);

    case DistortionType::Foldback:
    {
        float f = std::fmod(x - 1.0f, 4.0f);
        if (f < 0.0f) f += 4.0f;
        return std::abs(std::abs(f - 2.0f) - 1.0f) * 2.0f - 1.0f;
    }

    case DistortionType::Bitcrush:
    {
        float steps = std::pow(2.0f, static_cast<float>(std::clamp(bits, 2, 16)));
        return std::round(std::clamp(x, -1.0f, 1.0f) * steps) / steps;
    }

    default:
        return x;
    }
}

void AudioDistortionEffect::Reset()
{
    m_FilterL = 0.0f;
    m_FilterR = 0.0f;
}

void AudioDistortionEffect::Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate)
{
    if (!enabled || frameCount == 0)
    {
        std::memcpy(output, input, frameCount * channels * sizeof(float));
        return;
    }

    if (sampleRate == 0) sampleRate = 44100;

    float cutoffHz = 800.0f + std::clamp(tone, 0.0f, 1.0f) * 15000.0f;
    float dt = 1.0f / static_cast<float>(sampleRate);
    float rc = 1.0f / (2.0f * kPi * cutoffHz);
    float alpha = dt / (rc + dt);

    for (uint32_t f = 0; f < frameCount; ++f)
    {
        float inL = input[f * channels];
        float inR = (channels > 1) ? input[f * channels + 1] : inL;

        float distL = SaturateSample(inL * drive, type, bits);
        float distR = SaturateSample(inR * drive, type, bits);

        m_FilterL += alpha * (distL - m_FilterL);
        m_FilterR += alpha * (distR - m_FilterR);

        m_FilterL = DenormalGuard(m_FilterL);
        m_FilterR = DenormalGuard(m_FilterR);

        float wetL = m_FilterL * outputGain;
        float wetR = m_FilterR * outputGain;

        output[f * channels] = inL * (1.0f - mix) + wetL * mix;
        if (channels > 1)
            output[f * channels + 1] = inR * (1.0f - mix) + wetR * mix;

        for (uint32_t c = 2; c < channels; ++c)
            output[f * channels + c] = input[f * channels + c];
    }
}

json AudioDistortionEffect::Serialize() const
{
    json j = AudioEffect::Serialize();
    j["distortionType"] = static_cast<int>(type);
    j["drive"] = drive;
    j["mix"] = mix;
    j["tone"] = tone;
    j["outputGain"] = outputGain;
    j["bits"] = bits;
    return j;
}

void AudioDistortionEffect::Deserialize(const json& j)
{
    AudioEffect::Deserialize(j);
    if (j.contains("distortionType")) type = static_cast<DistortionType>(j["distortionType"].get<int>());
    if (j.contains("drive"))          drive = j["drive"].get<float>();
    if (j.contains("mix"))            mix = j["mix"].get<float>();
    if (j.contains("tone"))           tone = j["tone"].get<float>();
    if (j.contains("outputGain"))     outputGain = j["outputGain"].get<float>();
    if (j.contains("bits"))           bits = j["bits"].get<int>();
}

AudioDelayEffect::AudioDelayEffect()
{
    EnsureBufferSize(44100);
}

void AudioDelayEffect::EnsureBufferSize(uint32_t sampleRate)
{
    if (sampleRate == 0) sampleRate = 44100;
    if (m_CurrentSampleRate == sampleRate && !m_BufferL.empty()) return;

    m_CurrentSampleRate = sampleRate;
    size_t maxFrames = static_cast<size_t>(sampleRate * 2.1f);
    m_BufferL.assign(maxFrames, 0.0f);
    m_BufferR.assign(maxFrames, 0.0f);
    m_WriteIndex = 0;
    m_FilterL = 0.0f;
    m_FilterR = 0.0f;
}

void AudioDelayEffect::Reset()
{
    std::fill(m_BufferL.begin(), m_BufferL.end(), 0.0f);
    std::fill(m_BufferR.begin(), m_BufferR.end(), 0.0f);
    m_WriteIndex = 0;
    m_FilterL = 0.0f;
    m_FilterR = 0.0f;
}

void AudioDelayEffect::Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate)
{
    if (!enabled || frameCount == 0)
    {
        std::memcpy(output, input, frameCount * channels * sizeof(float));
        return;
    }

    EnsureBufferSize(sampleRate);

    size_t bufSize = m_BufferL.size();
    float delaySamples = std::clamp(delayTime, 0.01f, 2.0f) * static_cast<float>(sampleRate);
    float clampedFeedback = std::clamp(feedback, 0.0f, 0.95f);

    float dampCoeff = 0.25f;

    for (uint32_t f = 0; f < frameCount; ++f)
    {
        float inL = input[f * channels];
        float inR = (channels > 1) ? input[f * channels + 1] : inL;

        float readPos = static_cast<float>(m_WriteIndex) - delaySamples;
        while (readPos < 0.0f) readPos += static_cast<float>(bufSize);

        size_t idx0 = static_cast<size_t>(readPos) % bufSize;
        size_t idx1 = (idx0 + 1) % bufSize;
        float frac = readPos - std::floor(readPos);

        float delayedL = m_BufferL[idx0] + frac * (m_BufferL[idx1] - m_BufferL[idx0]);
        float delayedR = m_BufferR[idx0] + frac * (m_BufferR[idx1] - m_BufferR[idx0]);

        delayedL = DenormalGuard(delayedL);
        delayedR = DenormalGuard(delayedR);

        m_FilterL = (delayedL * (1.0f - dampCoeff)) + (m_FilterL * dampCoeff);
        m_FilterR = (delayedR * (1.0f - dampCoeff)) + (m_FilterR * dampCoeff);

        m_FilterL = DenormalGuard(m_FilterL);
        m_FilterR = DenormalGuard(m_FilterR);

        if (pingPong)
        {
            m_BufferL[m_WriteIndex] = inL + m_FilterR * clampedFeedback;
            m_BufferR[m_WriteIndex] = inR + m_FilterL * clampedFeedback;
        }
        else
        {
            m_BufferL[m_WriteIndex] = inL + m_FilterL * clampedFeedback;
            m_BufferR[m_WriteIndex] = inR + m_FilterR * clampedFeedback;
        }

        output[f * channels] = inL * (1.0f - mix) + delayedL * mix;
        if (channels > 1)
            output[f * channels + 1] = inR * (1.0f - mix) + delayedR * mix;

        for (uint32_t c = 2; c < channels; ++c)
            output[f * channels + c] = input[f * channels + c];

        if (++m_WriteIndex >= bufSize)
            m_WriteIndex = 0;
    }
}

json AudioDelayEffect::Serialize() const
{
    json j = AudioEffect::Serialize();
    j["delayTime"] = delayTime;
    j["feedback"] = feedback;
    j["mix"] = mix;
    j["pingPong"] = pingPong;
    return j;
}

void AudioDelayEffect::Deserialize(const json& j)
{
    AudioEffect::Deserialize(j);
    if (j.contains("delayTime")) delayTime = j["delayTime"].get<float>();
    if (j.contains("feedback"))  feedback  = j["feedback"].get<float>();
    if (j.contains("mix"))       mix       = j["mix"].get<float>();
    if (j.contains("pingPong"))  pingPong  = j["pingPong"].get<bool>();
}

void AudioFilterEffect::Reset()
{
    m_LpL = 0.0f; m_BpL = 0.0f;
    m_LpR = 0.0f; m_BpR = 0.0f;
}

void AudioFilterEffect::Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate)
{
    if (!enabled || frameCount == 0)
    {
        std::memcpy(output, input, frameCount * channels * sizeof(float));
        return;
    }

    if (sampleRate == 0) sampleRate = 44100;

    float clampedCutoff = std::clamp(cutoffFrequency, 20.0f, static_cast<float>(sampleRate) * 0.49f);
    float clampedQ = std::clamp(resonance, 0.1f, 10.0f);

    float f = 2.0f * std::sin(kPi * clampedCutoff / static_cast<float>(sampleRate));
    float q = 1.0f / clampedQ;

    for (uint32_t i = 0; i < frameCount; ++i)
    {
        float inL = input[i * channels];
        float inR = (channels > 1) ? input[i * channels + 1] : inL;

        float hpL = inL - m_LpL - q * m_BpL;
        m_BpL += f * hpL;
        m_LpL += f * m_BpL;

        float hpR = inR - m_LpR - q * m_BpR;
        m_BpR += f * hpR;
        m_LpR += f * m_BpR;

        m_LpL = DenormalGuard(m_LpL); m_BpL = DenormalGuard(m_BpL);
        m_LpR = DenormalGuard(m_LpR); m_BpR = DenormalGuard(m_BpR);

        float outL = 0.0f;
        float outR = 0.0f;

        switch (type)
        {
        case FilterType::LowPass:
            outL = m_LpL;
            outR = m_LpR;
            break;
        case FilterType::HighPass:
            outL = hpL;
            outR = hpR;
            break;
        case FilterType::BandPass:
            outL = m_BpL;
            outR = m_BpR;
            break;
        case FilterType::Notch:
            outL = hpL + m_LpL;
            outR = hpR + m_LpR;
            break;
        }

        output[i * channels] = outL;
        if (channels > 1)
            output[i * channels + 1] = outR;

        for (uint32_t c = 2; c < channels; ++c)
            output[i * channels + c] = input[i * channels + c];
    }
}

json AudioFilterEffect::Serialize() const
{
    json j = AudioEffect::Serialize();
    j["filterType"] = static_cast<int>(type);
    j["cutoffFrequency"] = cutoffFrequency;
    j["resonance"] = resonance;
    return j;
}

void AudioFilterEffect::Deserialize(const json& j)
{
    AudioEffect::Deserialize(j);
    if (j.contains("filterType"))      type = static_cast<FilterType>(j["filterType"].get<int>());
    if (j.contains("cutoffFrequency")) cutoffFrequency = j["cutoffFrequency"].get<float>();
    if (j.contains("resonance"))       resonance = j["resonance"].get<float>();
}

void AudioEqualizerEffect::Reset()
{
    m_LowL = 0.0f; m_MidL = 0.0f;
    m_LowR = 0.0f; m_MidR = 0.0f;
}

void AudioEqualizerEffect::Process(const float* input, float* output, uint32_t frameCount, uint32_t channels, uint32_t sampleRate)
{
    if (!enabled || frameCount == 0)
    {
        std::memcpy(output, input, frameCount * channels * sizeof(float));
        return;
    }

    if (sampleRate == 0) sampleRate = 44100;

    float lowGainLin = std::pow(10.0f, lowGainDb / 20.0f);
    float midGainLin = std::pow(10.0f, midGainDb / 20.0f);
    float highGainLin = std::pow(10.0f, highGainDb / 20.0f);

    float fLow = 2.0f * std::sin(kPi * std::clamp(lowCutoff, 20.0f, 1000.0f) / static_cast<float>(sampleRate));
    float fHigh = 2.0f * std::sin(kPi * std::clamp(highCutoff, 1000.0f, static_cast<float>(sampleRate) * 0.45f) / static_cast<float>(sampleRate));

    for (uint32_t i = 0; i < frameCount; ++i)
    {
        float inL = input[i * channels];
        float inR = (channels > 1) ? input[i * channels + 1] : inL;

        m_LowL += fLow * (inL - m_LowL);
        m_MidL += fHigh * (inL - m_MidL);
        m_LowL = DenormalGuard(m_LowL);
        m_MidL = DenormalGuard(m_MidL);

        float bandLowL = m_LowL;
        float bandMidL = m_MidL - m_LowL;
        float bandHighL = inL - m_MidL;

        m_LowR += fLow * (inR - m_LowR);
        m_MidR += fHigh * (inR - m_MidR);
        m_LowR = DenormalGuard(m_LowR);
        m_MidR = DenormalGuard(m_MidR);

        float bandLowR = m_LowR;
        float bandMidR = m_MidR - m_LowR;
        float bandHighR = inR - m_MidR;

        output[i * channels] = bandLowL * lowGainLin + bandMidL * midGainLin + bandHighL * highGainLin;
        if (channels > 1)
            output[i * channels + 1] = bandLowR * lowGainLin + bandMidR * midGainLin + bandHighR * highGainLin;

        for (uint32_t c = 2; c < channels; ++c)
            output[i * channels + c] = input[i * channels + c];
    }
}

json AudioEqualizerEffect::Serialize() const
{
    json j = AudioEffect::Serialize();
    j["lowGainDb"] = lowGainDb;
    j["midGainDb"] = midGainDb;
    j["highGainDb"] = highGainDb;
    j["lowCutoff"] = lowCutoff;
    j["highCutoff"] = highCutoff;
    return j;
}

void AudioEqualizerEffect::Deserialize(const json& j)
{
    AudioEffect::Deserialize(j);
    if (j.contains("lowGainDb"))  lowGainDb  = j["lowGainDb"].get<float>();
    if (j.contains("midGainDb"))  midGainDb  = j["midGainDb"].get<float>();
    if (j.contains("highGainDb")) highGainDb = j["highGainDb"].get<float>();
    if (j.contains("lowCutoff"))  lowCutoff  = j["lowCutoff"].get<float>();
    if (j.contains("highCutoff")) highCutoff = j["highCutoff"].get<float>();
}

std::shared_ptr<AudioEffect> CreateAudioEffect(AudioEffectType type)
{
    switch (type)
    {
    case AudioEffectType::Reverb:     return std::make_shared<AudioReverbEffect>();
    case AudioEffectType::Limiter:    return std::make_shared<AudioLimiterEffect>();
    case AudioEffectType::Distortion: return std::make_shared<AudioDistortionEffect>();
    case AudioEffectType::Delay:      return std::make_shared<AudioDelayEffect>();
    case AudioEffectType::Filter:     return std::make_shared<AudioFilterEffect>();
    case AudioEffectType::Equalizer:  return std::make_shared<AudioEqualizerEffect>();
    default:                          return nullptr;
    }
}

}
