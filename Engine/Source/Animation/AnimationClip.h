#pragma once
#include <string>
#include "Animation/AnimationTrack.h"
#include "Animation/LoopMode.h"

namespace rv::Animation
{

struct AnimationClip {
    std::string name = "New Animation";
    float duration = 0.0f;
    LoopMode loopMode = LoopMode::NONE;

    AnimationTrack<glm::vec3> positionTrack;
    AnimationTrack<glm::quat> rotationTrack;
    AnimationTrack<glm::vec3> scaleTrack;

    AnimationClip(const std::string& clipName = "New Animation");
    void RecalculateDuration();
};

}