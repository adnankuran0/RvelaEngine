#pragma once
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Asset/AssetRegistry.h"
#include "Core/Ref.h"
#include "json.hpp"
#include <fstream>

namespace rv {

class AnimationLibrarySerializer
{
public:
    static Ref<AnimationLibraryAsset> CreateNew(
        const std::filesystem::path& path,
        AssetRegistry& registry)
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

            // Tracks
            nlohmann::json tracksJson;

            // Position
            tracksJson["position"] = nlohmann::json::array();
            for (const auto& kf : clip->positionTrack.keyframes)
            {
                tracksJson["position"].push_back({
                    { "time", kf.time },
                    { "ease", static_cast<int>(kf.ease) },
                    { "value", { { "x", kf.value.x }, { "y", kf.value.y }, { "z", kf.value.z } } }
                    });
            }

            // Rotation
            tracksJson["rotation"] = nlohmann::json::array();
            for (const auto& kf : clip->rotationTrack.keyframes)
            {
                tracksJson["rotation"].push_back({
                    { "time", kf.time },
                    { "ease", static_cast<int>(kf.ease) },
                    { "value", { { "w", kf.value.w }, { "x", kf.value.x }, { "y", kf.value.y }, { "z", kf.value.z } } }
                    });
            }

            // Scale
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
            root["clips"].push_back(clipJson);
        }

        std::ofstream file(path);
        if (!file.is_open()) return false;

        file << root.dump(4);
        return true;
    }
};

}