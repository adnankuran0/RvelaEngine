#pragma once
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Asset/AssetRegistry.h"
#include "Animation/IPropertyTrack.h"
#include "Core/Ref.h"
#include "json.hpp"
#include <fstream>

namespace rv {

class AnimationLibrarySerializer
{
public:
    static Ref<AnimationLibraryAsset> CreateNew(const std::filesystem::path& path, AssetRegistry& registry)
    {
        AssetMeta meta = registry.GetOrCreateMeta(path);
        auto asset = CreateRef<AnimationLibraryAsset>(meta.uuid);

        Save(asset, path);
        registry.SaveMeta(path, meta);
        return asset;
    }

    static bool Save(const Ref<AnimationLibraryAsset>& asset, const std::filesystem::path& path)
    {
        if (!asset) return false;

        nlohmann::json root;
        root["clips"] = nlohmann::json::array();

        for (const auto& [name, clip] : asset->GetClips())
        {
            nlohmann::json clipJson;
            clipJson["name"] = clip->name;
            clipJson["duration"] = clip->duration;
            clipJson["loop_mode"] = static_cast<int>(clip->loopMode);

            // Events
            nlohmann::json eventsJson = nlohmann::json::array();
            for (const auto& ev : clip->eventTrack)
            {
                eventsJson.push_back({
                    { "time", ev.time },
                    { "name", ev.name },
                    { "parameter", ev.parameter }
                    });
            }
            clipJson["events"] = eventsJson;

            //Tracks
            nlohmann::json tracksJson;
            tracksJson["position"] = nlohmann::json::array();
            for (const auto& kf : clip->positionTrack.keyframes)
            {
                tracksJson["position"].push_back({
                    { "time", kf.time },
                    { "ease", static_cast<int>(kf.ease) },
                    { "value", { { "x", kf.value.x }, { "y", kf.value.y }, { "z", kf.value.z } } }
                    });
            }

            tracksJson["rotation"] = nlohmann::json::array();
            for (const auto& kf : clip->rotationTrack.keyframes)
            {
                tracksJson["rotation"].push_back({
                    { "time", kf.time },
                    { "ease", static_cast<int>(kf.ease) },
                    { "value", { { "w", kf.value.w }, { "x", kf.value.x }, { "y", kf.value.y }, { "z", kf.value.z } } }
                    });
            }

            tracksJson["scale"] = nlohmann::json::array();
            for (const auto& kf : clip->scaleTrack.keyframes)
            {
                tracksJson["scale"].push_back({
                    { "time", kf.time },
                    { "ease", static_cast<int>(kf.ease) },
                    { "value", { { "x", kf.value.x }, { "y", kf.value.y }, { "z", kf.value.z } } }
                    });
            }
            clipJson["tracks"] = tracksJson;

            nlohmann::json propTracksJson = nlohmann::json::array();
            for (const auto& propTrack : clip->propertyTracks)
            {
                nlohmann::json pJson;
                pJson["target_path"] = propTrack->targetPath;
                pJson["property_name"] = propTrack->propertyName;
                pJson["type"] = static_cast<int>(propTrack->GetType());

                nlohmann::json pKeys = nlohmann::json::array();

                if (propTrack->GetType() == Animation::PropertyType::Float) {
                    auto tTrack = std::static_pointer_cast<Animation::TypedPropertyTrack<float>>(propTrack);
                    for (const auto& kf : tTrack->track.keyframes) {
                        pKeys.push_back({ { "time", kf.time }, { "ease", static_cast<int>(kf.ease) }, { "value", kf.value } });
                    }
                }
                else if (propTrack->GetType() == Animation::PropertyType::Vec3) {
                    auto tTrack = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec3>>(propTrack);
                    for (const auto& kf : tTrack->track.keyframes) {
                        pKeys.push_back({ { "time", kf.time }, { "ease", static_cast<int>(kf.ease) },
                            { "value", { { "x", kf.value.x }, { "y", kf.value.y }, { "z", kf.value.z } } } });
                    }
                }
                else if (propTrack->GetType() == Animation::PropertyType::Vec4) {
                    auto tTrack = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::vec4>>(propTrack);
                    for (const auto& kf : tTrack->track.keyframes) {
                        pKeys.push_back({ { "time", kf.time }, { "ease", static_cast<int>(kf.ease) },
                            { "value", { { "x", kf.value.x }, { "y", kf.value.y }, { "z", kf.value.z }, { "w", kf.value.w } } } });
                    }
                }
                else if (propTrack->GetType() == Animation::PropertyType::Quat) {
                    auto tTrack = std::static_pointer_cast<Animation::TypedPropertyTrack<glm::quat>>(propTrack);
                    for (const auto& kf : tTrack->track.keyframes) {
                        pKeys.push_back({ { "time", kf.time }, { "ease", static_cast<int>(kf.ease) },
                            { "value", { { "w", kf.value.w }, { "x", kf.value.x }, { "y", kf.value.y }, { "z", kf.value.z } } } });
                    }
                }

                pJson["keyframes"] = pKeys;
                propTracksJson.push_back(pJson);
            }
            clipJson["property_tracks"] = propTracksJson;

            root["clips"].push_back(clipJson);
        }

        std::ofstream file(path);
        if (!file.is_open()) return false;

        file << root.dump(4);
        return true;
    }
};

}