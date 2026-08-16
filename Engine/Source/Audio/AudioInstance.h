#pragma once
#include "miniaudio/miniaudio.h"
#include "entt/entt.h"

namespace rv {

enum class PlaybackState
{
    Stopped,
    Playing,
    Paused
};

struct AudioInstance
{
    ma_sound sound{};
    ma_audio_buffer_ref bufferRef;
    uint32_t busID = 0;
    PlaybackState state = PlaybackState::Stopped;
};

} // namespace rv