#pragma once
#include <string>
#include "Animation/AnimationTrack.h"

namespace rv::Animation
{

    struct AnimationClip {
        std::string name = "New Animation";
        float duration = 0.0f;
        bool isLooping = true;

        AnimationTrack<glm::vec3> positionTrack;
        AnimationTrack<glm::quat> rotationTrack;
        AnimationTrack<glm::vec3> scaleTrack;

        AnimationClip(const std::string& clipName = "New Animation");
        void RecalculateDuration();
    };
}