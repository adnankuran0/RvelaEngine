#pragma once
#include <vector>
#include <string>
#include "Animation/InterpolationType.h"
#include "Animation/Keyframe.h"
#include "Animation/AnimationUtils.h"

namespace rv::Animation
{

template<typename T>
struct AnimationTrack
{
	std::string targetProperty;
	InterpolationType interpolation = InterpolationType::LINEAR;
	std::vector<Keyframe<T>> keyframes;

    void AddKeyframe(float time, const T& value) {
        auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time,
            [](const Keyframe<T>& kf, float t) { return kf.time < t; });

        if (it != keyframes.end() && std::abs(it->time - time) < 0.0001f) {
            it->value = value;
        }
        else {
            keyframes.insert(it, Keyframe<T>(time, value));
        }
    }

    T Sample(float time) const {
        if (keyframes.empty()) return T{};
        if (keyframes.size() == 1 || time <= keyframes.front().time) return keyframes.front().value;
        if (time >= keyframes.back().time) return keyframes.back().value;

        auto it = std::upper_bound(keyframes.begin(), keyframes.end(), time,
            [](float t, const Keyframe<T>& kf) { return t < kf.time; });

        size_t nextIdx = std::distance(keyframes.begin(), it);
        size_t prevIdx = nextIdx - 1;

        const auto& prevKey = keyframes[prevIdx];
        const auto& nextKey = keyframes[nextIdx];

        float delta = nextKey.time - prevKey.time;
        float alpha = (delta > 0.0f) ? (time - prevKey.time) / delta : 0.0f;
        alpha = std::clamp(alpha, 0.0f, 1.0f);

        return Interpolate(prevKey.value, nextKey.value, alpha, interpolation);
    }


};

}