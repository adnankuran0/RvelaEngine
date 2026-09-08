#include "rvelapch.h"
#include "AnimationLibrarySerializer.h"
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Animation/BonePropertyTrack.h"
#include "Asset/CacheTypes/AnimationLibraryHeader.h"
#include "Core/Log.h"
#include <fstream>

using namespace rv;
using namespace rv::Animation;

static void WriteStringBinary(std::ofstream& file, const std::string& str)
{
    uint32_t len = static_cast<uint32_t>(str.size());
    file.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0)
        file.write(str.data(), len);
}

Ref<AnimationLibraryAsset> AnimationLibrarySerializer::CreateNew(const std::filesystem::path& path, AssetRegistry& registry)
{
    AssetMeta meta = registry.GetOrCreateMeta(path);
    auto asset = CreateRef<AnimationLibraryAsset>(meta.uuid);

    Save(asset, path);
    registry.SaveMeta(path, meta);
    return asset;
}

bool AnimationLibrarySerializer::Save(const Ref<AnimationLibraryAsset>& asset, const std::filesystem::path& path)
{
    if (!asset) return false;

    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Cannot write binary animation library: {}", path.string());
        return false;
    }

    const auto& clips = asset->GetClips();

    AnimationLibraryHeader header{};
    header.magic = MAGIC_ANIM_LIB;
    header.version = ANIM_LIB_VERSION;
    header.clipCount = static_cast<uint32_t>(clips.size());
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    for (const auto& [name, clip] : clips)
    {
        WriteStringBinary(file, clip->name);
        file.write(reinterpret_cast<const char*>(&clip->duration), sizeof(clip->duration));

        int32_t loopMode = static_cast<int32_t>(clip->loopMode);
        file.write(reinterpret_cast<const char*>(&loopMode), sizeof(loopMode));

        // Events
        uint32_t eventCount = static_cast<uint32_t>(clip->eventTrack.size());
        file.write(reinterpret_cast<const char*>(&eventCount), sizeof(eventCount));
        for (const auto& ev : clip->eventTrack)
        {
            file.write(reinterpret_cast<const char*>(&ev.time), sizeof(ev.time));
            WriteStringBinary(file, ev.name);
            WriteStringBinary(file, ev.parameter);
        }

        // Property Tracks
        uint32_t trackCount = static_cast<uint32_t>(clip->propertyTracks.size());
        file.write(reinterpret_cast<const char*>(&trackCount), sizeof(trackCount));

        // AnimationLibrarySerializer::Save içindeki propertyTracks döngüsü:
        for (const auto& track : clip->propertyTracks)
        {
            WriteStringBinary(file, track->targetPath);
            WriteStringBinary(file, track->propertyName);

            uint32_t propType = static_cast<uint32_t>(track->GetType());
            file.write(reinterpret_cast<const char*>(&propType), sizeof(propType));

            switch (track->GetType())
            {
            case PropertyType::BoneVec3:
            {
                auto bTrack = std::static_pointer_cast<BonePropertyTrack<glm::vec3>>(track);
                uint32_t transType = static_cast<uint32_t>(bTrack->transformType);
                file.write(reinterpret_cast<const char*>(&transType), sizeof(transType));

                uint32_t kfCount = static_cast<uint32_t>(bTrack->track.keyframes.size());
                file.write(reinterpret_cast<const char*>(&kfCount), sizeof(kfCount));
                file.write(reinterpret_cast<const char*>(bTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::vec3>));
                break;
            }
            case PropertyType::BoneQuat:
            {
                auto bTrack = std::static_pointer_cast<BonePropertyTrack<glm::quat>>(track);
                uint32_t transType = static_cast<uint32_t>(bTrack->transformType);
                file.write(reinterpret_cast<const char*>(&transType), sizeof(transType));

                uint32_t kfCount = static_cast<uint32_t>(bTrack->track.keyframes.size());
                file.write(reinterpret_cast<const char*>(&kfCount), sizeof(kfCount));
                file.write(reinterpret_cast<const char*>(bTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::quat>));
                break;
            }
            case PropertyType::Float:
            {
                auto tTrack = std::static_pointer_cast<TypedPropertyTrack<float>>(track);
                uint32_t kfCount = static_cast<uint32_t>(tTrack->track.keyframes.size());
                file.write(reinterpret_cast<const char*>(&kfCount), sizeof(kfCount));
                file.write(reinterpret_cast<const char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<float>));
                break;
            }
            case PropertyType::Vec3:
            {
                auto tTrack = std::static_pointer_cast<TypedPropertyTrack<glm::vec3>>(track);
                uint32_t kfCount = static_cast<uint32_t>(tTrack->track.keyframes.size());
                file.write(reinterpret_cast<const char*>(&kfCount), sizeof(kfCount));
                file.write(reinterpret_cast<const char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::vec3>));
                break;
            }
            case PropertyType::Vec4:
            {
                auto tTrack = std::static_pointer_cast<TypedPropertyTrack<glm::vec4>>(track);
                uint32_t kfCount = static_cast<uint32_t>(tTrack->track.keyframes.size());
                file.write(reinterpret_cast<const char*>(&kfCount), sizeof(kfCount));
                file.write(reinterpret_cast<const char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::vec4>));
                break;
            }
            case PropertyType::Quat:
            {
                auto tTrack = std::static_pointer_cast<TypedPropertyTrack<glm::quat>>(track);
                uint32_t kfCount = static_cast<uint32_t>(tTrack->track.keyframes.size());
                file.write(reinterpret_cast<const char*>(&kfCount), sizeof(kfCount));
                file.write(reinterpret_cast<const char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::quat>));
                break;
            }
            case PropertyType::Bool:
            {
                auto tTrack = std::static_pointer_cast<TypedPropertyTrack<bool>>(track);
                uint32_t kfCount = static_cast<uint32_t>(tTrack->track.keyframes.size());
                file.write(reinterpret_cast<const char*>(&kfCount), sizeof(kfCount));
                file.write(reinterpret_cast<const char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<bool>));
                break;
            }
            }
        }
    }

    return file.good();
}