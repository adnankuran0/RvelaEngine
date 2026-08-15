#include "rvelapch.h"
#include "sol/sol.hpp"
#include "AudioBindings.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioBus.h"
#include "Scene/Components/AudioEmitterComponent.h"
#include "Asset/AssetManager.h"
#include "Asset/Types/AudioClipAsset.h"

using namespace rv;

void rv::LuaBindings::RegisterAudioAPI(sol::state& lua)
{
    lua.new_enum<AttenuationModel>("AttenuationModel", {
        { "None",        AttenuationModel::None        },
        { "Inverse",     AttenuationModel::Inverse     },
        { "Linear",      AttenuationModel::Linear      },
        { "Exponential", AttenuationModel::Exponential }
        });

    lua.new_usertype<AudioEmitterComponent>("AudioEmitterComponent",
        "volume", sol::property(
            [](AudioEmitterComponent& c) { return c.volume; },
            [](AudioEmitterComponent& c, float v) { AudioManager::Get().SetVolume(&c, v); }),

        "pitch", sol::property(
            [](AudioEmitterComponent& c) { return c.pitch; },
            [](AudioEmitterComponent& c, float p) { AudioManager::Get().SetPitch(&c, p); }),

        "loop", sol::property(
            [](AudioEmitterComponent& c) { return c.loop; },
            [](AudioEmitterComponent& c, bool l) { AudioManager::Get().SetLoop(&c, l); }),

        "spatial", sol::property(
            [](AudioEmitterComponent& c) { return c.spatial; },
            [](AudioEmitterComponent& c, bool s) { AudioManager::Get().SetSpatial(&c, s); }),

        "attenuationModel", sol::property(
            [](AudioEmitterComponent& c) { return c.attenuationModel; },
            [](AudioEmitterComponent& c, AttenuationModel m) { AudioManager::Get().SetAttenuationModel(&c, m); }),

        "minDistance", sol::property(
            [](AudioEmitterComponent& c) { return c.minDistance; },
            [](AudioEmitterComponent& c, float d) { AudioManager::Get().SetMinDistance(&c, d); }),

        "maxDistance", sol::property(
            [](AudioEmitterComponent& c) { return c.maxDistance; },
            [](AudioEmitterComponent& c, float d) { AudioManager::Get().SetMaxDistance(&c, d); }),

        "rolloff", sol::property(
            [](AudioEmitterComponent& c) { return c.rolloff; },
            [](AudioEmitterComponent& c, float r) { AudioManager::Get().SetRolloff(&c, r); }),

        "dopplerFactor", sol::property(
            [](AudioEmitterComponent& c) { return c.dopplerFactor; },
            [](AudioEmitterComponent& c, float f) { AudioManager::Get().SetDopplerFactor(&c, f); }),

        "busID", sol::property(
            [](AudioEmitterComponent& c) { return c.busID; },
            [](AudioEmitterComponent& c, uint32_t id) { AudioManager::Get().SetBus(&c, id); }),

        "playOnCreate", &AudioEmitterComponent::playOnCreate,

        "Play", [](AudioEmitterComponent& c) { AudioManager::Get().Play(&c); },
        "Stop", [](AudioEmitterComponent& c) { AudioManager::Get().Stop(&c); },
        "Pause", [](AudioEmitterComponent& c) { AudioManager::Get().Pause(&c); },
        "Resume", [](AudioEmitterComponent& c) { AudioManager::Get().Resume(&c); },

        "IsPlaying", [](AudioEmitterComponent& c) { return AudioManager::Get().IsPlaying(&c); },
        "IsPaused", [](AudioEmitterComponent& c) { return AudioManager::Get().IsPaused(&c); },

        "Seek", [](AudioEmitterComponent& c, float seconds) { AudioManager::Get().Seek(&c, seconds); },
        "GetPlaybackPosition", [](AudioEmitterComponent& c) { return AudioManager::Get().GetPlaybackPosition(&c); },

        "SetClip", [](AudioEmitterComponent& c, const AssetUUID& uuid) {
            AudioManager::Get().SetClip(&c, AssetManager::Get().GetAsset<AudioClipAsset>(uuid));
        }
    );

    sol::table audio = lua.create_named_table("Audio");
    audio.set_function("SetBusVolume", [](uint32_t busID, float volume) { AudioManager::Get().SetBusVolume(busID, volume); });
    audio.set_function("GetBusVolume", [](uint32_t busID) { AudioManager::Get().GetBusVolume(busID); });
    audio.set_function("GetBusID", [](const std::string& name) { return AudioManager::Get().GetBusID(name); });
    //audio.set_function("CreateBus", []() -> uint32_t { return AudioManager::Get().CreateBus()->GetID(); });
    //audio.set_function("DestroyBus", [](uint32_t busID) { return AudioManager::Get().DestroyBus(busID); });
    audio.set_function("SetParentBus", [](uint32_t busID, uint32_t parentID) { return AudioManager::Get().SetParentBus(busID, parentID); });
}