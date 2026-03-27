#pragma once
#include "json.hpp"

namespace rv {

using json = nlohmann::json;

struct AudioEmitterComponent
{
    // Ref<AudioClipAsset>
    std::string path;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
    bool spatial = true;
    bool playOnCreate = false;

    bool recreate = true;

    uint32_t instanceID = UINT32_MAX;

    json Serialize() const;
    void Deserialize(const json& j);
};

}