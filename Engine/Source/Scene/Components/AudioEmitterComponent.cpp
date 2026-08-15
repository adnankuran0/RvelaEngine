#include "rvelapch.h"
#include "AudioEmitterComponent.h"

using namespace rv;

json AudioEmitterComponent::Serialize() const
{
    json j;

    j["audioClip"] = audioClipUUID.ToString();
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

    j["busID"] = busID;


    return j;
}

void AudioEmitterComponent::Deserialize(const json& j)
{
    audioClipUUID = AssetUUID::FromString(j.at("audioClip").get<std::string>());
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

    busID = j["busID"];

    recreate = true;
}