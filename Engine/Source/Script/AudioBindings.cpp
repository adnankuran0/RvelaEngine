#include "rvelapch.h"
#include "sol/sol.hpp"
#include "AudioBindings.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioBus.h"
#include "Scene/Components/AudioEmitterComponent.h"
#include "Asset/AssetManager.h"
#include "Asset/Types/AudioClipAsset.h"
#include "ComponentHandle.h"

using namespace rv;

using AudioEmitterHandle = ComponentHandle<AudioEmitterComponent>;

void rv::LuaBindings::RegisterAudioAPI(sol::state& lua)
{
    lua.new_enum<AttenuationModel>("AttenuationModel", {
        { "None",        AttenuationModel::None        },
        { "Inverse",     AttenuationModel::Inverse     },
        { "Linear",      AttenuationModel::Linear      },
        { "Exponential", AttenuationModel::Exponential }
        });

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
        }
    );

    sol::table audio = lua.create_named_table("Audio");
    audio.set_function("SetBusVolume", [](uint32_t busID, float volume) { AudioManager::Get().SetBusVolume(busID, volume); });
    audio.set_function("GetBusVolume", [](uint32_t busID) { AudioManager::Get().GetBusVolume(busID); });
    audio.set_function("GetBusID", [](const std::string& name) { return AudioManager::Get().GetBusID(name); });
    audio.set_function("SetParentBus", [](uint32_t busID, uint32_t parentID) { return AudioManager::Get().SetParentBus(busID, parentID); });
}