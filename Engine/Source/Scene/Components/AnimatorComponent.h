#pragma once
#include "json.hpp"
#include "Asset/AssetUUID.h"
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Animation/AnimationClip.h"
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Core/Ref.h"

namespace rv {

using json = nlohmann::json;

struct AnimatorComponent
{
    AssetUUID libraryUUID;
    std::shared_ptr<Animation::AnimationClip> currentClip = nullptr;
    std::string currentClipName = "";
    float playbackSpeed = 1.0f;
    bool autoplay = false;

    //runtime
    Ref<AnimationLibraryAsset> library;
    float currentTime = 0.0f;
    bool isPlaying = false;
    bool isStarted = false;

    bool isBlending = false;
    bool blendSnapshotTaken = false;
    float blendDuration = 0.0f;
    float blendElapsed = 0.0f;

    glm::vec3 blendFromPosition{ 0.0f };
    glm::quat blendFromRotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 blendFromScale{ 1.0f };

    std::vector<glm::vec3> blendFromBonePositions;
    std::vector<glm::quat> blendFromBoneRotations;
    std::vector<glm::vec3> blendFromBoneScales;

    AnimatorComponent() = default;
    AnimatorComponent(AssetUUID libUUID) { SetLibrary(libUUID); }

    void Play() { isPlaying = true; isStarted = false; }
    void Play(const std::string& clipName, float blendTime = 0.0f);
    void Pause() { isPlaying = false; }
    void Stop() { isPlaying = false; currentTime = 0.0f; isBlending = false; }

    void SetLibrary(AssetUUID uuid);
    void SetClip(const std::string& clipName);

    json Serialize() const;
    void Deserialize(const json& j);
};

}