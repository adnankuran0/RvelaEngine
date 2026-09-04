#include "rvelapch.h"
#include "AnimationLibraryLoader.h"
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Animation/EaseType.h"
#include "Animation/IPropertyTrack.h"
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

static glm::vec4 ParseVec4(const json& j)
{
    return { j.value("x", 0.0f), j.value("y", 0.0f), j.value("z", 0.0f), j.value("w", 1.0f) };
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

        if (clipJson.contains("events") && clipJson["events"].is_array())
        {
            for (const auto& evJson : clipJson["events"])
            {
                float time = evJson.value("time", 0.0f);
                std::string name = evJson.value("name", "");
                std::string param = evJson.value("parameter", "");
                clip->AddEvent(time, name, param);
            }
        }

        if (clipJson.contains("tracks"))
        {
            const auto& tracksJson = clipJson["tracks"];

            if (tracksJson.contains("position") && tracksJson["position"].is_array())
            {
                for (const auto& kf : tracksJson["position"])
                {
                    float time = kf.value("time", 0.0f);
                    auto ease = static_cast<Animation::EaseType>(kf.value("ease", 0));
                    glm::vec3 val = ParseVec3(kf["value"]);
                    clip->positionTrack.AddKeyframe(time, val, ease);
                }
            }

            if (tracksJson.contains("rotation") && tracksJson["rotation"].is_array())
            {
                for (const auto& kf : tracksJson["rotation"])
                {
                    float time = kf.value("time", 0.0f);
                    auto ease = static_cast<Animation::EaseType>(kf.value("ease", 0));
                    glm::quat val = ParseQuat(kf["value"]);
                    clip->rotationTrack.AddKeyframe(time, val, ease);
                }
            }

            if (tracksJson.contains("scale") && tracksJson["scale"].is_array())
            {
                for (const auto& kf : tracksJson["scale"])
                {
                    float time = kf.value("time", 0.0f);
                    auto ease = static_cast<Animation::EaseType>(kf.value("ease", 0));
                    glm::vec3 val = ParseVec3(kf["value"]);
                    clip->scaleTrack.AddKeyframe(time, val, ease);
                }
            }
        }

        if (clipJson.contains("property_tracks") && clipJson["property_tracks"].is_array())
        {
            for (const auto& pJson : clipJson["property_tracks"])
            {
                std::string targetPath = pJson.value("target_path", "");
                std::string propName = pJson.value("property_name", "");
                auto type = static_cast<Animation::PropertyType>(pJson.value("type", 0));

                if (type == Animation::PropertyType::Float)
                {
                    auto pTrack = std::make_shared<Animation::TypedPropertyTrack<float>>();
                    pTrack->targetPath = targetPath;
                    pTrack->propertyName = propName;
                    for (const auto& kf : pJson["keyframes"]) {
                        pTrack->track.AddKeyframe(kf.value("time", 0.0f), kf.value("value", 0.0f), static_cast<Animation::EaseType>(kf.value("ease", 0)));
                    }
                    clip->propertyTracks.push_back(pTrack);
                }
                else if (type == Animation::PropertyType::Vec3)
                {
                    auto pTrack = std::make_shared<Animation::TypedPropertyTrack<glm::vec3>>();
                    pTrack->targetPath = targetPath;
                    pTrack->propertyName = propName;
                    for (const auto& kf : pJson["keyframes"]) {
                        pTrack->track.AddKeyframe(kf.value("time", 0.0f), ParseVec3(kf["value"]), static_cast<Animation::EaseType>(kf.value("ease", 0)));
                    }
                    clip->propertyTracks.push_back(pTrack);
                }
                else if (type == Animation::PropertyType::Vec4)
                {
                    auto pTrack = std::make_shared<Animation::TypedPropertyTrack<glm::vec4>>();
                    pTrack->targetPath = targetPath;
                    pTrack->propertyName = propName;
                    for (const auto& kf : pJson["keyframes"]) {
                        pTrack->track.AddKeyframe(kf.value("time", 0.0f), ParseVec4(kf["value"]), static_cast<Animation::EaseType>(kf.value("ease", 0)));
                    }
                    clip->propertyTracks.push_back(pTrack);
                }
                else if (type == Animation::PropertyType::Quat)
                {
                    auto pTrack = std::make_shared<Animation::TypedPropertyTrack<glm::quat>>();
                    pTrack->targetPath = targetPath;
                    pTrack->propertyName = propName;
                    for (const auto& kf : pJson["keyframes"]) {
                        pTrack->track.AddKeyframe(kf.value("time", 0.0f), ParseQuat(kf["value"]), static_cast<Animation::EaseType>(kf.value("ease", 0)));
                    }
                    clip->propertyTracks.push_back(pTrack);
                }
            }
        }

        libraryAsset->AddClip(clip);
    }

    return libraryAsset;
}