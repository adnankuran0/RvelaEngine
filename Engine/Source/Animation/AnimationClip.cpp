#include "rvelapch.h"
#include "Animation/AnimationClip.h"

using namespace rv::Animation;

AnimationClip::AnimationClip(const std::string& clipName)
    : name(clipName), duration(0.0f) {
    positionTrack.targetProperty = "Position";
    rotationTrack.targetProperty = "Rotation";
    scaleTrack.targetProperty = "Scale";
}

AnimationClip::AnimationClip(const AnimationClip& other)
    : name(other.name),
    duration(other.duration),
    loopMode(other.loopMode),
    positionTrack(other.positionTrack),
    rotationTrack(other.rotationTrack),
    scaleTrack(other.scaleTrack),
    eventTrack(other.eventTrack)
{
    for (const auto& track : other.propertyTracks) {
        if (track) {
            propertyTracks.push_back(track->Clone());
        }
    }
}

void AnimationClip::AddEvent(float time, const std::string& name, const std::string& parameter)
{
    auto it = std::lower_bound(eventTrack.begin(), eventTrack.end(), time,
        [](const AnimationEvent& ev, float t) { return ev.time < t; });
    eventTrack.insert(it, AnimationEvent(time, name, parameter));
}