#include "rvelapch.h"
#include "sol/sol.hpp"
#include "AnimationBindings.h"
#include "Animation/LoopMode.h"
#include "Animation/EaseType.h"
#include "ComponentHandle.h" 
#include "Scene/Components/AnimatorComponent.h"


using AnimatorHandle = rv::ComponentHandle<rv::AnimatorComponent>;

using namespace rv::Animation;

void rv::LuaBindings::RegisterAnimationAPI(sol::state& lua)
{
    lua.new_enum<LoopMode>("LoopMode", {
        { "None",     LoopMode::NONE     },
        { "Linear",   LoopMode::LINEAR   },
        { "PingPong", LoopMode::PINGPONG }
        });

    lua.new_enum<EaseType>("EaseType", {
        { "Linear",    EaseType::LINEAR      },
        { "EaseIn",    EaseType::EASE_IN     },
        { "EaseOut",   EaseType::EASE_OUT    },
        { "EaseInOut", EaseType::EASE_IN_OUT },
        { "EaseOutIn", EaseType::EASE_OUT_IN },
        { "Zero",      EaseType::ZERO        }
        });

    lua.new_usertype<AnimatorHandle>("AnimatorComponent",
        "IsValid", &AnimatorHandle::IsValid,

        // functions
        "Play", sol::overload(
            [](AnimatorHandle& h) {
                if (auto* c = h.Get()) c->Play();
            },
            [](AnimatorHandle& h, const std::string& clipName) {
                if (auto* c = h.Get()) {
                    c->SetClip(clipName);
                    c->Play();
                }
            }
        ),
        "Pause", [](AnimatorHandle& h) {
            if (auto* c = h.Get()) c->Pause();
        },
        "Stop", [](AnimatorHandle& h) {
            if (auto* c = h.Get()) c->Stop();
        },
        "SetClip", [](AnimatorHandle& h, const std::string& clipName) {
            if (auto* c = h.Get()) c->SetClip(clipName);
        },

        // properties
        "isPlaying", sol::property(
            [](AnimatorHandle& h) {
                return h.Get() ? h.Get()->isPlaying : false;
            },
            [](AnimatorHandle& h, bool playing) {
                if (auto* c = h.Get()) {
                    if (playing) c->Play();
                    else c->Pause();
                }
            }
        ),
        "currentTime", sol::property(
            [](AnimatorHandle& h) { return h.Get() ? h.Get()->currentTime : 0.0f; },
            [](AnimatorHandle& h, float time) { if (auto* c = h.Get()) c->currentTime = time; }
        ),
        "speed", sol::property(
            [](AnimatorHandle& h) { return h.Get() ? h.Get()->playbackSpeed : 1.0f; },
            [](AnimatorHandle& h, float spd) { if (auto* c = h.Get()) c->playbackSpeed = spd; }
        ),
        "currentClipName", sol::property(
            [](AnimatorHandle& h) { return h.Get() ? h.Get()->currentClipName : ""; }
        ),

        // read only
        "duration", sol::property([](AnimatorHandle& h) -> float {
            if (auto* c = h.Get()) {
                return c->currentClip ? c->currentClip->duration : 0.0f;
            }
            return 0.0f;
            }),
        "loopMode", sol::property(
            [](AnimatorHandle& h) -> LoopMode {
                if (auto* c = h.Get()) {
                    return c->currentClip ? c->currentClip->loopMode : LoopMode::NONE;
                }
                return LoopMode::NONE;
            },
            [](AnimatorHandle& h, LoopMode mode) {
                if (auto* c = h.Get()) {
                    if (c->currentClip) c->currentClip->loopMode = mode;
                }
            }
        ),
        "hasClip", [](AnimatorHandle& h, const std::string& clipName) -> bool {
            if (auto* c = h.Get()) {
                return c->library ? c->library->HasClip(clipName) : false;
            }
            return false;
        }
    );
}