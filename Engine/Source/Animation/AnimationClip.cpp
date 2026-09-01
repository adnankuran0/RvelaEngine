#include "rvelapch.h"
#include "Animation/AnimationClip.h"

using namespace rv::Animation;

AnimationClip::AnimationClip(const std::string& clipName)
    : name(clipName), duration(0.0f), isLooping(false) {
    positionTrack.targetProperty = "Position";
    rotationTrack.targetProperty = "Rotation";
    scaleTrack.targetProperty = "Scale";
}

void AnimationClip::RecalculateDuration() {
    duration = 0.0f;
    if (!positionTrack.keyframes.empty()) {
        duration = std::max(duration, positionTrack.keyframes.back().time);
    }
    if (!rotationTrack.keyframes.empty()) {
        duration = std::max(duration, rotationTrack.keyframes.back().time);
    }
    if (!scaleTrack.keyframes.empty()) {
        duration = std::max(duration, scaleTrack.keyframes.back().time);
    }
}