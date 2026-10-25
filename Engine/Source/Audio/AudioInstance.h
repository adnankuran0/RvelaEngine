#pragma once
#include "miniaudio/miniaudio.h"
#include "entt/entt.h"
#include "Audio/AudioEffects.h"
#include <vector>
#include <mutex>
#include <cstring>
#include <algorithm>

namespace rv {

struct AudioEmitterComponent;
struct AudioInstance;

enum class PlaybackState
{
    Stopped,
    Playing,
    Paused,
    Finished
};

struct EmitterEffectNode
{
    ma_node_base base;
    AudioInstance* instance = nullptr;
};

struct AudioInstance
{
    ma_sound sound{};
    ma_audio_buffer_ref bufferRef;
    uint32_t busID = 0;
    PlaybackState state = PlaybackState::Stopped;

    EmitterEffectNode effectNode{};
    bool effectNodeInitialized = false;
    AudioEmitterComponent* emitterComp = nullptr;
    std::vector<float> intermediateBuffer;
    std::mutex effectMutex;

    void ProcessEffects(const float* in, float* out, uint32_t frameCount, uint32_t channels, uint32_t sampleRate);
};

} // namespace rv
