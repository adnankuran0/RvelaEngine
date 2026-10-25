#pragma once
#include "Audio/AudioEffects.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace rv::UI {

void DrawAudioEffectsList(
    const std::string& id,
    std::vector<std::shared_ptr<AudioEffect>>& effects,
    const std::function<void(std::shared_ptr<AudioEffect>)>& onAddEffect = nullptr,
    const std::function<void(size_t)>& onRemoveEffect = nullptr,
    const std::function<void(size_t, size_t)>& onMoveEffect = nullptr);

void DrawAudioEffectDetails(AudioEffect* effect);

}
