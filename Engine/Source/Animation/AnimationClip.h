#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Animation/AnimationTrack.h"
#include "Animation/LoopMode.h"
#include "Animation/AnimationEvent.h"
#include "Animation/IPropertyTrack.h"

namespace rv::Animation {

    struct AnimationClip {
        std::string name = "New Animation";
        float duration = 0.0f;
        LoopMode loopMode = LoopMode::NONE;

        AnimationTrack<glm::vec3> positionTrack;
        AnimationTrack<glm::quat> rotationTrack;
        AnimationTrack<glm::vec3> scaleTrack;
        std::vector<AnimationEvent> eventTrack;

        std::vector<std::shared_ptr<IPropertyTrack>> propertyTracks;

        AnimationClip(const std::string& clipName = "New Animation");
        void AddEvent(float time, const std::string& name, const std::string& parameter = "");
    };

}