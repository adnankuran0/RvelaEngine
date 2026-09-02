#include "rvelapch.h"
#include "AnimationLibraryLoader.h"
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Core/Log.h"
#include "json.hpp"
#include <fstream>
#include "Asset/AssetMeta.h"

using namespace rv;
using json = nlohmann::json;

static glm::vec3 ParseVec3(const json& j)
{
    return { j.value("x", 0.0f), j.value("y", 0.0f), j.value("z", 0.0f) };
}

static glm::quat ParseQuat(const json& j)
{
    return glm::quat(
        j.value("w", 1.0f),
        j.value("x", 0.0f),
        j.value("y", 0.0f),
        j.value("z", 0.0f)
    );
}

Ref<Asset> AnimationLibraryLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath);
    if (!file)
    {
        LOG_ERROR("Cannot open animation library: {}", assetPath.string());
        return nullptr;
    }

    json root;
    try
    {
        file >> root;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("JSON parse error in {}: {}", assetPath.string(), e.what());
        return nullptr;
    }

    auto libraryAsset = CreateRef<AnimationLibraryAsset>(meta.uuid);

    if (!root.contains("clips") || !root["clips"].is_array())
    {
        LOG_WARN("Animation library has no valid 'clips' array: {}", assetPath.string());
        return libraryAsset;
    }

    for (const auto& clipJson : root["clips"])
    {
        std::string clipName = clipJson.value("name", "Unnamed Clip");
        auto clip = std::make_shared<Animation::AnimationClip>(clipName);

        clip->duration = clipJson.value("duration", 0.0f);
        clip->loopMode = static_cast<Animation::LoopMode>(clipJson.value("loop_mode", 0));

        if (clipJson.contains("tracks"))
        {
            const auto& tracksJson = clipJson["tracks"];

            // Position Track
            if (tracksJson.contains("position") && tracksJson["position"].is_array())
            {
                for (const auto& kf : tracksJson["position"])
                {
                    float time = kf.value("time", 0.0f);
                    glm::vec3 val = ParseVec3(kf["value"]);
                    clip->positionTrack.AddKeyframe(time, val);
                }
            }

            // Rotation Track
            if (tracksJson.contains("rotation") && tracksJson["rotation"].is_array())
            {
                for (const auto& kf : tracksJson["rotation"])
                {
                    float time = kf.value("time", 0.0f);
                    glm::quat val = ParseQuat(kf["value"]);
                    clip->rotationTrack.AddKeyframe(time, val);
                }
            }

            // Scale Track
            if (tracksJson.contains("scale") && tracksJson["scale"].is_array())
            {
                for (const auto& kf : tracksJson["scale"])
                {
                    float time = kf.value("time", 0.0f);
                    glm::vec3 val = ParseVec3(kf["value"]);
                    clip->scaleTrack.AddKeyframe(time, val);
                }
            }
        }

        clip->RecalculateDuration();
        libraryAsset->AddClip(clip);
    }

    return libraryAsset;
}