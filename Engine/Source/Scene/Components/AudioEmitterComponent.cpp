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

    json fxArr = json::array();
    for (const auto& fx : effects)
    {
        if (fx)
            fxArr.push_back(fx->Serialize());
    }
    j["effects"] = fxArr;

    return j;
}

void AudioEmitterComponent::Deserialize(const json& j)
{
    if (j.contains("audioClip"))
        audioClipUUID = AssetUUID::FromString(j.at("audioClip").get<std::string>());
    if (j.contains("volume")) volume = j["volume"];
    if (j.contains("pitch")) pitch = j["pitch"];
    if (j.contains("loop")) loop = j["loop"];
    if (j.contains("playOnCreate")) playOnCreate = j["playOnCreate"];

    if (j.contains("spatial")) spatial = j["spatial"];
    if (j.contains("attenuationModel")) attenuationModel = static_cast<AttenuationModel>(j["attenuationModel"]);
    if (j.contains("minDistance")) minDistance = j["minDistance"];
    if (j.contains("maxDistance")) maxDistance = j["maxDistance"];
    if (j.contains("rolloff")) rolloff = j["rolloff"];

    if (j.contains("doppler")) doppler = j["doppler"];
    if (j.contains("dopplerFactor")) dopplerFactor = j["dopplerFactor"];

    if (j.contains("busID")) busID = j["busID"];

    effects.clear();
    if (j.contains("effects") && j["effects"].is_array())
    {
        for (const auto& fxJson : j["effects"])
        {
            if (!fxJson.contains("type")) continue;
            auto type = static_cast<AudioEffectType>(fxJson["type"].get<int>());
            auto fx = CreateAudioEffect(type);
            if (fx)
            {
                fx->Deserialize(fxJson);
                effects.push_back(fx);
            }
        }
    }
}
