#pragma once
#include "json.hpp"

namespace rv {

using json = nlohmann::json;

enum class AttenuationModel
{
    None = 0,
    Inverse = 1,
    Linear = 2,
    Exponential = 3
};

struct AudioEmitterComponent
{
    // Ref<AudioClipAsset>
    std::string path;

    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
    bool playOnCreate = false;
    bool spatial = true;

    AttenuationModel attenuationModel = AttenuationModel::Inverse;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
    float rolloff = 1.0f;

    bool doppler = true;
    float dopplerFactor = 1.0f;

    uint32_t busID = 0;

    // local variables
    bool recreate = true;
    uint32_t instanceID = UINT32_MAX;

    glm::vec3 prevWorldPos = { 0.0f, 0.0, 0.0 };
    bool prevPosValid = false;

    json Serialize() const;
    void Deserialize(const json& j);
};

}