#include "rvelapch.h"
#include "sol/sol.hpp"
#include "AnimationBindings.h"
#include "Scene/Components/AnimatorComponent.h"
#include "Animation/LoopMode.h"
#include "Animation/EaseType.h"

using namespace rv::Animation;

void rv::LuaBindings::RegisterAnimationAPI(sol::state& lua)
{
    lua.new_enum<LoopMode>("LoopMode", {
        { "None",     LoopMode::NONE     },
        { "Linear",   LoopMode::LINEAR   },
        { "PingPong", LoopMode::PINGPONG }
        });

    lua.new_enum<EaseType>("EaseType", {
        { "Linear",    EaseType::LINEAR     },
        { "EaseIn",    EaseType::EASE_IN    },
        { "EaseOut",   EaseType::EASE_OUT   },
        { "EaseInOut", EaseType::EASE_IN_OUT },
        { "EaseOutIn", EaseType::EASE_OUT_IN },
        { "Zero",      EaseType::ZERO       }
        });

    lua.new_usertype<AnimatorComponent>("AnimatorComponent",
        // functions
        "Play", sol::overload(
            [](AnimatorComponent& c) { c.Play(); },
            [](AnimatorComponent& c, const std::string& clipName) {
                c.SetClip(clipName);
                c.Play();
            }
        ),
        "Pause", &AnimatorComponent::Pause,
        "Stop", &AnimatorComponent::Stop,
        "SetClip", &AnimatorComponent::SetClip,

        // properties
        "isPlaying", sol::property(
            [](AnimatorComponent& c) { return c.isPlaying; },
            [](AnimatorComponent& c, bool playing) {
                if (playing) c.Play();
                else c.Pause();
            }
        ),
        "currentTime", sol::property(
            [](AnimatorComponent& c) { return c.currentTime; },
            [](AnimatorComponent& c, float time) { c.currentTime = time; }
        ),
        "speed", sol::property(
            [](AnimatorComponent& c) { return c.playbackSpeed; },
            [](AnimatorComponent& c, float spd) { c.playbackSpeed = spd; }
        ),
        "currentClipName", sol::property([](AnimatorComponent& c) { return c.currentClipName; }),

        // read only
        "duration", sol::property([](AnimatorComponent& c) -> float {
            return c.currentClip ? c.currentClip->duration : 0.0f;
            }),
        "loopMode", sol::property(
            [](AnimatorComponent& c) -> LoopMode {
                return c.currentClip ? c.currentClip->loopMode : LoopMode::NONE;
            },
            [](AnimatorComponent& c, LoopMode mode) {
                if (c.currentClip) c.currentClip->loopMode = mode;
            }
        ),
        "hasClip", [](AnimatorComponent& c, const std::string& clipName) -> bool {
            return c.library ? c.library->HasClip(clipName) : false;
        }
    );
}