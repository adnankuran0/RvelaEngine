#include "rvelapch.h"
#include "AnimatorComponent.h"
#include "Asset/AssetManager.h"

using namespace rv;

void AnimatorComponent::SetLibrary(AssetUUID uuid)
{
    libraryUUID = uuid;
    if (!uuid.IsValid())
    {
        library = nullptr;
        currentClip = nullptr;
        currentClipName = "";
        return;
    }

    library = AssetManager::Get().GetAsset<AnimationLibraryAsset>(uuid);
    if (library && !library->GetClips().empty())
    {
        if (!currentClipName.empty() && library->HasClip(currentClipName))
        {
            SetClip(currentClipName);
        }
        else
        {
            SetClip(library->GetClips().begin()->first);
        }
    }
    else
    {
        currentClip = nullptr;
        currentClipName = "";
    }
}

void AnimatorComponent::SetClip(const std::string& clipName)
{
    if (!library) return;

    auto clip = library->GetClip(clipName);
    if (clip)
    {
        currentClip = clip;
        currentClipName = clipName;
        currentTime = 0.0f;
    }

    isStarted = false;
}

json AnimatorComponent::Serialize() const
{
    json j;
    j["libraryUUID"] = libraryUUID.ToString();
    j["currentClipName"] = currentClipName;
    j["playbackSpeed"] = playbackSpeed;
    j["autoplay"] = autoplay;
    return j;
}

void AnimatorComponent::Deserialize(const json& j)
{
    if (j.contains("libraryUUID"))
    {
        std::string uuidStr = j["libraryUUID"];
        SetLibrary(AssetUUID::FromString(uuidStr));
    }
    if (j.contains("currentClipName"))
    {
        std::string clipName = j["currentClipName"];
        SetClip(clipName);
    }
    if (j.contains("autoplay"))
    {
        autoplay = j["autoplay"];
    }
    playbackSpeed = j.value("playbackSpeed", 1.0f);
}
