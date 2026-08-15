#pragma once
#include "miniaudio/miniaudio.h"
#include "entt/entt.h"

namespace rv {

struct AudioInstance
{
    ma_sound sound{};
    ma_audio_buffer_ref bufferRef;
    uint32_t busID = 0;
};

} // namespace rv