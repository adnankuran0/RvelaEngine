#pragma once
#include "json.hpp"
#include "Animation/AnimationClip.h"
#include <memory>

namespace rv {

using json = nlohmann::json;

struct AnimatorComponent
{
    std::shared_ptr<Animation::AnimationClip> currentClip = nullptr;
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool isPlaying = true;

    AnimatorComponent();
    AnimatorComponent(std::shared_ptr<Animation::AnimationClip> clip)
        : currentClip(clip){
    }

    void Play() { isPlaying = true; }
    void Pause() { isPlaying = false; }
    void Stop() { isPlaying = false; currentTime = 0.0f; }

    void SetClip(std::shared_ptr<Animation::AnimationClip> clip)
    {
        currentClip = clip;
        currentTime = 0.0f;
    }

    json Serialize() const;
    void Deserialize(const json& j);

};

}