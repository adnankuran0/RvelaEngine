#include "rvelapch.h"
#include "sol/sol.hpp"
#include "AudioBindings.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioBus.h"
#include "Audio/AudioEffects.h"
#include "Scene/Components/AudioEmitterComponent.h"
#include "Asset/AssetManager.h"
#include "Asset/Types/AudioClipAsset.h"
#include "ComponentHandle.h"

using namespace rv;

using AudioEmitterHandle = ComponentHandle<AudioEmitterComponent>;

static sol::object ToLuaEffect(sol::this_state s, const std::shared_ptr<AudioEffect>& fx)
{
    if (!fx) return sol::nil;
    sol::state_view lua(s);
    switch (fx->GetType())
    {
    case AudioEffectType::Filter:
        return sol::make_object(lua, std::static_pointer_cast<AudioFilterEffect>(fx));
    case AudioEffectType::Reverb:
        return sol::make_object(lua, std::static_pointer_cast<AudioReverbEffect>(fx));
    case AudioEffectType::Delay:
        return sol::make_object(lua, std::static_pointer_cast<AudioDelayEffect>(fx));
    case AudioEffectType::Distortion:
        return sol::make_object(lua, std::static_pointer_cast<AudioDistortionEffect>(fx));
    case AudioEffectType::Limiter:
        return sol::make_object(lua, std::static_pointer_cast<AudioLimiterEffect>(fx));
    case AudioEffectType::Equalizer:
        return sol::make_object(lua, std::static_pointer_cast<AudioEqualizerEffect>(fx));
    default:
        return sol::make_object(lua, fx);
    }
}

void rv::LuaBindings::RegisterAudioAPI(sol::state& lua)
{
    lua.new_enum<AttenuationModel>("AttenuationModel", {
        { "None",        AttenuationModel::None        },
        { "Inverse",     AttenuationModel::Inverse     },
        { "Linear",      AttenuationModel::Linear      },
        { "Exponential", AttenuationModel::Exponential }
    });

    lua.new_enum<AudioEffectType>("AudioEffectType", {
        { "Reverb",     AudioEffectType::Reverb     },
        { "Limiter",    AudioEffectType::Limiter    },
        { "Distortion", AudioEffectType::Distortion },
        { "Delay",      AudioEffectType::Delay      },
        { "Filter",     AudioEffectType::Filter     },
        { "Equalizer",  AudioEffectType::Equalizer  }
    });

    lua.new_enum<DistortionType>("DistortionType", {
        { "SoftClip", DistortionType::SoftClip },
        { "HardClip", DistortionType::HardClip },
        { "Tanh",     DistortionType::Tanh     },
        { "Foldback", DistortionType::Foldback },
        { "Bitcrush", DistortionType::Bitcrush }
    });

    lua.new_enum<FilterType>("FilterType", {
        { "LowPass",  FilterType::LowPass  },
        { "HighPass", FilterType::HighPass },
        { "BandPass", FilterType::BandPass },
        { "Notch",    FilterType::Notch    }
    });

    // Audio Effect Types
    lua.new_usertype<AudioEffect>("AudioEffect",
        sol::no_constructor,
        "enabled", &AudioEffect::enabled,
        "GetType", &AudioEffect::GetType
    );

    lua.new_usertype<AudioFilterEffect>("AudioFilterEffect",
        sol::no_constructor,
        sol::base_classes, sol::bases<AudioEffect>(),
        "enabled", &AudioFilterEffect::enabled,
        "GetType", &AudioFilterEffect::GetType,
        "type", &AudioFilterEffect::type,
        "filterType", &AudioFilterEffect::type,
        "cutoff", &AudioFilterEffect::cutoffFrequency,
        "cutoffFrequency", &AudioFilterEffect::cutoffFrequency,
        "resonance", &AudioFilterEffect::resonance,
        "q", &AudioFilterEffect::resonance
    );

    lua.new_usertype<AudioReverbEffect>("AudioReverbEffect",
        sol::no_constructor,
        sol::base_classes, sol::bases<AudioEffect>(),
        "enabled", &AudioReverbEffect::enabled,
        "GetType", &AudioReverbEffect::GetType,
        "roomSize", &AudioReverbEffect::roomSize,
        "damping", &AudioReverbEffect::damping,
        "width", &AudioReverbEffect::width,
        "wet", &AudioReverbEffect::wet,
        "dry", &AudioReverbEffect::dry,
        "freeze", &AudioReverbEffect::freeze
    );

    lua.new_usertype<AudioDelayEffect>("AudioDelayEffect",
        sol::no_constructor,
        sol::base_classes, sol::bases<AudioEffect>(),
        "enabled", &AudioDelayEffect::enabled,
        "GetType", &AudioDelayEffect::GetType,
        "delayTime", &AudioDelayEffect::delayTime,
        "feedback", &AudioDelayEffect::feedback,
        "mix", &AudioDelayEffect::mix,
        "pingPong", &AudioDelayEffect::pingPong
    );

    lua.new_usertype<AudioDistortionEffect>("AudioDistortionEffect",
        sol::no_constructor,
        sol::base_classes, sol::bases<AudioEffect>(),
        "enabled", &AudioDistortionEffect::enabled,
        "GetType", &AudioDistortionEffect::GetType,
        "type", &AudioDistortionEffect::type,
        "distortionType", &AudioDistortionEffect::type,
        "drive", &AudioDistortionEffect::drive,
        "tone", &AudioDistortionEffect::tone,
        "bits", &AudioDistortionEffect::bits
    );

    lua.new_usertype<AudioLimiterEffect>("AudioLimiterEffect",
        sol::no_constructor,
        sol::base_classes, sol::bases<AudioEffect>(),
        "enabled", &AudioLimiterEffect::enabled,
        "GetType", &AudioLimiterEffect::GetType,
        "thresholdDb", &AudioLimiterEffect::thresholdDb,
        "ceilingDb", &AudioLimiterEffect::ceilingDb,
        "releaseMs", &AudioLimiterEffect::releaseMs,
        "makeupGainDb", &AudioLimiterEffect::makeupGainDb,
        "currentGainReduction", sol::readonly(&AudioLimiterEffect::currentGainReduction)
    );

    lua.new_usertype<AudioEqualizerEffect>("AudioEqualizerEffect",
        sol::no_constructor,
        sol::base_classes, sol::bases<AudioEffect>(),
        "enabled", &AudioEqualizerEffect::enabled,
        "GetType", &AudioEqualizerEffect::GetType,
        "lowGainDb", &AudioEqualizerEffect::lowGainDb,
        "midGainDb", &AudioEqualizerEffect::midGainDb,
        "highGainDb", &AudioEqualizerEffect::highGainDb,
        "lowCutoff", &AudioEqualizerEffect::lowCutoff,
        "highCutoff", &AudioEqualizerEffect::highCutoff
    );

    lua.new_usertype<AudioEmitterHandle>("AudioEmitterComponent",
        "IsValid", &AudioEmitterHandle::IsValid,

        "volume", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->volume : 0.0f; },
            [](AudioEmitterHandle& h, float v) { if (auto* c = h.Get()) AudioManager::Get().SetVolume(c, v); }),

        "pitch", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->pitch : 1.0f; },
            [](AudioEmitterHandle& h, float p) { if (auto* c = h.Get()) AudioManager::Get().SetPitch(c, p); }),

        "loop", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->loop : false; },
            [](AudioEmitterHandle& h, bool l) { if (auto* c = h.Get()) AudioManager::Get().SetLoop(c, l); }),

        "spatial", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->spatial : false; },
            [](AudioEmitterHandle& h, bool s) { if (auto* c = h.Get()) AudioManager::Get().SetSpatial(c, s); }),

        "attenuationModel", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->attenuationModel : AttenuationModel::None; },
            [](AudioEmitterHandle& h, AttenuationModel m) { if (auto* c = h.Get()) AudioManager::Get().SetAttenuationModel(c, m); }),

        "minDistance", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->minDistance : 0.0f; },
            [](AudioEmitterHandle& h, float d) { if (auto* c = h.Get()) AudioManager::Get().SetMinDistance(c, d); }),

        "maxDistance", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->maxDistance : 0.0f; },
            [](AudioEmitterHandle& h, float d) { if (auto* c = h.Get()) AudioManager::Get().SetMaxDistance(c, d); }),

        "rolloff", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->rolloff : 0.0f; },
            [](AudioEmitterHandle& h, float r) { if (auto* c = h.Get()) AudioManager::Get().SetRolloff(c, r); }),

        "dopplerFactor", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->dopplerFactor : 0.0f; },
            [](AudioEmitterHandle& h, float f) { if (auto* c = h.Get()) AudioManager::Get().SetDopplerFactor(c, f); }),

        "busID", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->busID : 0; },
            [](AudioEmitterHandle& h, uint32_t id) { if (auto* c = h.Get()) AudioManager::Get().SetBus(c, id); }),

        "playOnCreate", sol::property(
            [](AudioEmitterHandle& h) { return h.Get() ? h.Get()->playOnCreate : false; },
            [](AudioEmitterHandle& h, bool b) { if (auto* c = h.Get()) c->playOnCreate = b; }),

        "Play", [](AudioEmitterHandle& h) { if (auto* c = h.Get()) AudioManager::Get().Play(c); },
        "Stop", [](AudioEmitterHandle& h) { if (auto* c = h.Get()) AudioManager::Get().Stop(c); },
        "Pause", [](AudioEmitterHandle& h) { if (auto* c = h.Get()) AudioManager::Get().Pause(c); },
        "Resume", [](AudioEmitterHandle& h) { if (auto* c = h.Get()) AudioManager::Get().Resume(c); },

        "IsPlaying", [](AudioEmitterHandle& h) { return h.Get() ? AudioManager::Get().IsPlaying(h.Get()) : false; },
        "IsPaused", [](AudioEmitterHandle& h) { return h.Get() ? AudioManager::Get().IsPaused(h.Get()) : false; },

        "Seek", [](AudioEmitterHandle& h, float seconds) { if (auto* c = h.Get()) AudioManager::Get().Seek(c, seconds); },
        "GetPlaybackPosition", [](AudioEmitterHandle& h) { return h.Get() ? AudioManager::Get().GetPlaybackPosition(h.Get()) : 0.0f; },

        "SetClip", [](AudioEmitterHandle& h, const AssetUUID& uuid) {
            if (auto* c = h.Get()) {
                AudioManager::Get().SetClip(c, AssetManager::Get().GetAsset<AudioClipAsset>(uuid));
            }
        },

        "AddEffect", [](sol::this_state s, AudioEmitterHandle& h, AudioEffectType type) -> sol::object {
            if (auto* c = h.Get()) {
                auto fx = CreateAudioEffect(type);
                if (fx) {
                    AudioManager::Get().AddEmitterEffect(c, fx);
                    return ToLuaEffect(s, fx);
                }
            }
            return sol::nil;
        },

        "GetEffect", [](sol::this_state s, AudioEmitterHandle& h, size_t index) -> sol::object {
            if (auto* c = h.Get()) {
                if (index < c->effects.size()) {
                    return ToLuaEffect(s, c->effects[index]);
                }
            }
            return sol::nil;
        },

        "GetEffectByType", [](sol::this_state s, AudioEmitterHandle& h, AudioEffectType type) -> sol::object {
            if (auto* c = h.Get()) {
                for (const auto& fx : c->effects) {
                    if (fx && fx->GetType() == type) {
                        return ToLuaEffect(s, fx);
                    }
                }
            }
            return sol::nil;
        },

        "GetFilterEffect", [](sol::this_state s, AudioEmitterHandle& h) -> sol::object {
            if (auto* c = h.Get()) {
                for (const auto& fx : c->effects) {
                    if (fx && fx->GetType() == AudioEffectType::Filter) {
                        return ToLuaEffect(s, fx);
                    }
                }
            }
            return sol::nil;
        },

        "GetEffectCount", [](AudioEmitterHandle& h) -> size_t {
            if (auto* c = h.Get()) {
                return c->effects.size();
            }
            return 0;
        },

        "GetEffects", [](sol::this_state s, AudioEmitterHandle& h) -> sol::table {
            sol::state_view lua(s);
            sol::table tbl = lua.create_table();
            if (auto* c = h.Get()) {
                for (size_t i = 0; i < c->effects.size(); ++i) {
                    tbl[i + 1] = ToLuaEffect(s, c->effects[i]);
                }
            }
            return tbl;
        },

        "RemoveEffect", [](AudioEmitterHandle& h, size_t index) {
            if (auto* c = h.Get()) {
                AudioManager::Get().RemoveEmitterEffect(c, index);
            }
        },

        "ClearEffects", [](AudioEmitterHandle& h) {
            if (auto* c = h.Get()) {
                c->effects.clear();
            }
        }
    );

    sol::table audio = lua.create_named_table("Audio");
    audio.set_function("SetBusVolume", [](uint32_t busID, float volume) { AudioManager::Get().SetBusVolume(busID, volume); });
    audio.set_function("GetBusVolume", [](uint32_t busID) { return AudioManager::Get().GetBusVolume(busID); });
    audio.set_function("GetBusID", [](const std::string& name) { return AudioManager::Get().GetBusID(name); });
    audio.set_function("SetParentBus", [](uint32_t busID, uint32_t parentID) { return AudioManager::Get().SetParentBus(busID, parentID); });

    audio.set_function("AddBusEffect", [](sol::this_state s, uint32_t busID, AudioEffectType type) -> sol::object {
        auto fx = CreateAudioEffect(type);
        if (fx) {
            AudioManager::Get().AddBusEffect(busID, fx);
            return ToLuaEffect(s, fx);
        }
        return sol::nil;
    });

    audio.set_function("GetBusEffect", [](sol::this_state s, uint32_t busID, size_t index) -> sol::object {
        if (auto* bus = AudioManager::Get().GetBus(busID)) {
            const auto& effects = bus->GetEffects();
            if (index < effects.size()) {
                return ToLuaEffect(s, effects[index]);
            }
        }
        return sol::nil;
    });

    audio.set_function("GetBusEffectByType", [](sol::this_state s, uint32_t busID, AudioEffectType type) -> sol::object {
        if (auto* bus = AudioManager::Get().GetBus(busID)) {
            for (const auto& fx : bus->GetEffects()) {
                if (fx && fx->GetType() == type) {
                    return ToLuaEffect(s, fx);
                }
            }
        }
        return sol::nil;
    });

    audio.set_function("GetBusFilterEffect", [](sol::this_state s, uint32_t busID) -> sol::object {
        if (auto* bus = AudioManager::Get().GetBus(busID)) {
            for (const auto& fx : bus->GetEffects()) {
                if (fx && fx->GetType() == AudioEffectType::Filter) {
                    return ToLuaEffect(s, fx);
                }
            }
        }
        return sol::nil;
    });

    audio.set_function("GetBusEffectCount", [](uint32_t busID) -> size_t {
        if (auto* bus = AudioManager::Get().GetBus(busID)) {
            return bus->GetEffects().size();
        }
        return 0;
    });

    audio.set_function("GetBusEffects", [](sol::this_state s, uint32_t busID) -> sol::table {
        sol::state_view lua(s);
        sol::table tbl = lua.create_table();
        if (auto* bus = AudioManager::Get().GetBus(busID)) {
            const auto& effects = bus->GetEffects();
            for (size_t i = 0; i < effects.size(); ++i) {
                tbl[i + 1] = ToLuaEffect(s, effects[i]);
            }
        }
        return tbl;
    });

    audio.set_function("RemoveBusEffect", [](uint32_t busID, size_t index) {
        AudioManager::Get().RemoveBusEffect(busID, index);
    });
}
