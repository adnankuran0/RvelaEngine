#pragma once
#include "json.hpp"
#include "Asset/AssetUUID.h"
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Animation/AnimationClip.h"
#include <memory>
#include <string>
#include "Core/Ref.h"

namespace rv {

using json = nlohmann::json;

struct AnimatorComponent
{
    AssetUUID libraryUUID;
    std::string currentClipName = "";

    Ref<AnimationLibraryAsset> library;
    std::shared_ptr<Animation::AnimationClip> currentClip = nullptr;

    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool autoplay = false;
    bool isPlaying = false;

    AnimatorComponent() = default;
    AnimatorComponent(AssetUUID libUUID) { SetLibrary(libUUID); }

    void Play() { isPlaying = true; }
    void Pause() { isPlaying = false; }
    void Stop() { isPlaying = false; currentTime = 0.0f; }

    void SetLibrary(AssetUUID uuid);
    void SetClip(const std::string& clipName);

    json Serialize() const;
    void Deserialize(const json& j);
};

}