#pragma once

namespace rv::Animation
{

template <typename T>
struct Keyframe
{
	float time = 0.0f;
	T value{};

	Keyframe() = default;
	Keyframe(float t, const T& val) : time(t), value(val) {}
};

}