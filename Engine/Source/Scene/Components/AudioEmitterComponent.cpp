#include "rvelapch.h"
#include "AudioEmitterComponent.h"

using namespace rv;

json AudioEmitterComponent::Serialize() const
{
    json j;

    j["path"] = path;
    j["volume"] = volume;
    j["pitch"] = pitch;
    j["loop"] = loop;
    j["playOnCreate"] = playOnCreate;

    j["spatial"] = spatial;
    j["attenuationModel"] = static_cast<int>(attenuationModel);
    j["minDistance"] = minDistance;
    j["maxDistance"] = maxDistance;
    j["rolloff"] = rolloff;

    j["doppler"] = doppler;
    j["dopplerFactor"] = dopplerFactor;

    return j;
}

void AudioEmitterComponent::Deserialize(const json& j)
{
    path = j["path"];
    volume = j["volume"];
    pitch = j["pitch"];
    loop = j["loop"];
    playOnCreate = j["playOnCreate"];

    spatial = j["spatial"];
    attenuationModel = static_cast<AttenuationModel>(j["attenuationModel"]);
    minDistance = j["minDistance"];
    maxDistance = j["maxDistance"];
    rolloff = j["rolloff"];

    doppler = j["doppler"];
    dopplerFactor = j["dopplerFactor"];

    recreate = true;
}