#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "Animation/AnimationTrack.h"
#include "Animation/LoopMode.h"
#include "Animation/AnimationEvent.h"

namespace rv::Animation
{

struct AnimationClip {
    std::string name = "New Animation";
    float duration = 0.0f;
    LoopMode loopMode = LoopMode::NONE;

    AnimationTrack<glm::vec3> positionTrack;
    AnimationTrack<glm::quat> rotationTrack;
    AnimationTrack<glm::vec3> scaleTrack;

    std::vector<AnimationEvent> eventTrack;

    AnimationClip(const std::string& clipName = "New Animation");
    void RecalculateDuration();

    void AddEvent(float time, const std::string& name, const std::string& parameter = "");
};

}