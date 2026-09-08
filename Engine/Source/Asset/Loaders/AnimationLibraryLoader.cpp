#include "rvelapch.h"
#include "AnimationLibraryLoader.h"
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Animation/BonePropertyTrack.h"
#include "Animation/EaseType.h"
#include "Asset/CacheTypes/AnimationLibraryHeader.h"
#include "Core/Log.h"
#include "Asset/AssetMeta.h"
#include <fstream>

using namespace rv;
using namespace rv::Animation;

static std::string ReadStringBinary(std::ifstream& file)
{
    uint32_t len = 0;
    file.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len == 0) return "";
    std::string str(len, '\0');
    file.read(str.data(), len);
    return str;
}

Ref<Asset> AnimationLibraryLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Cannot open animation library: {}", assetPath.string());
        return nullptr;
    }

    AnimationLibraryHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file)
    {
        LOG_ERROR("Failed to read header: {}", assetPath.string());
        return nullptr;
    }

    if (header.magic != MAGIC_ANIM_LIB)
    {
        LOG_ERROR("Invalid magic: {}", assetPath.string());
        return nullptr;
    }

    if (header.version != ANIM_LIB_VERSION)
    {
        LOG_ERROR("Version mismatch (got {}, expected {}): {}",
            header.version, ANIM_LIB_VERSION, assetPath.string());
        return nullptr;
    }

    auto libraryAsset = CreateRef<AnimationLibraryAsset>(meta.uuid);

    for (uint32_t c = 0; c < header.clipCount; ++c)
    {
        std::string clipName = ReadStringBinary(file);
        auto clip = std::make_shared<AnimationClip>(clipName);

        file.read(reinterpret_cast<char*>(&clip->duration), sizeof(clip->duration));

        int32_t loopMode = 0;
        file.read(reinterpret_cast<char*>(&loopMode), sizeof(loopMode));
        clip->loopMode = static_cast<LoopMode>(loopMode);

        // Events
        uint32_t eventCount = 0;
        file.read(reinterpret_cast<char*>(&eventCount), sizeof(eventCount));
        for (uint32_t e = 0; e < eventCount; ++e)
        {
            float time = 0.0f;
            file.read(reinterpret_cast<char*>(&time), sizeof(time));
            std::string evName = ReadStringBinary(file);
            std::string evParam = ReadStringBinary(file);
            clip->AddEvent(time, evName, evParam);
        }

        // Property Tracks
        uint32_t trackCount = 0;
        file.read(reinterpret_cast<char*>(&trackCount), sizeof(trackCount));

        for (uint32_t t = 0; t < trackCount; ++t)
        {
            std::string targetPath = ReadStringBinary(file);
            std::string propName = ReadStringBinary(file);

            uint32_t propTypeVal = 0;
            file.read(reinterpret_cast<char*>(&propTypeVal), sizeof(propTypeVal));
            auto propType = static_cast<PropertyType>(propTypeVal);

            if (propType == PropertyType::BoneVec3)
            {
                uint32_t transType = 0;
                file.read(reinterpret_cast<char*>(&transType), sizeof(transType));

                uint32_t kfCount = 0;
                file.read(reinterpret_cast<char*>(&kfCount), sizeof(kfCount));

                auto bTrack = std::make_shared<BonePropertyTrack<glm::vec3>>();
                bTrack->targetPath = targetPath;
                bTrack->propertyName = propName;
                bTrack->transformType = static_cast<BoneTransformType>(transType);
                bTrack->track.keyframes.resize(kfCount);
                file.read(reinterpret_cast<char*>(bTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::vec3>));
                clip->propertyTracks.push_back(bTrack);
            }
            else if (propType == PropertyType::BoneQuat)
            {
                uint32_t transType = 0;
                file.read(reinterpret_cast<char*>(&transType), sizeof(transType));

                uint32_t kfCount = 0;
                file.read(reinterpret_cast<char*>(&kfCount), sizeof(kfCount));

                auto bTrack = std::make_shared<BonePropertyTrack<glm::quat>>();
                bTrack->targetPath = targetPath;
                bTrack->propertyName = propName;
                bTrack->transformType = static_cast<BoneTransformType>(transType);
                bTrack->track.keyframes.resize(kfCount);
                file.read(reinterpret_cast<char*>(bTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::quat>));
                clip->propertyTracks.push_back(bTrack);
            }
            else if (propType == PropertyType::Float)
            {
                uint32_t kfCount = 0;
                file.read(reinterpret_cast<char*>(&kfCount), sizeof(kfCount));
                auto tTrack = std::make_shared<TypedPropertyTrack<float>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                file.read(reinterpret_cast<char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<float>));
                clip->propertyTracks.push_back(tTrack);
            }
            else if (propType == PropertyType::Vec3)
            {
                uint32_t kfCount = 0;
                file.read(reinterpret_cast<char*>(&kfCount), sizeof(kfCount));
                auto tTrack = std::make_shared<TypedPropertyTrack<glm::vec3>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                file.read(reinterpret_cast<char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::vec3>));
                clip->propertyTracks.push_back(tTrack);
            }
            else if (propType == PropertyType::Vec4)
            {
                uint32_t kfCount = 0;
                file.read(reinterpret_cast<char*>(&kfCount), sizeof(kfCount));
                auto tTrack = std::make_shared<TypedPropertyTrack<glm::vec4>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                file.read(reinterpret_cast<char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::vec4>));
                clip->propertyTracks.push_back(tTrack);
            }
            else if (propType == PropertyType::Quat)
            {
                uint32_t kfCount = 0;
                file.read(reinterpret_cast<char*>(&kfCount), sizeof(kfCount));
                auto tTrack = std::make_shared<TypedPropertyTrack<glm::quat>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                file.read(reinterpret_cast<char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<glm::quat>));
                clip->propertyTracks.push_back(tTrack);
            }
            else if (propType == PropertyType::Bool)
            {
                uint32_t kfCount = 0;
                file.read(reinterpret_cast<char*>(&kfCount), sizeof(kfCount));
                auto tTrack = std::make_shared<TypedPropertyTrack<bool>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                file.read(reinterpret_cast<char*>(tTrack->track.keyframes.data()), kfCount * sizeof(Keyframe<bool>));
                clip->propertyTracks.push_back(tTrack);
            }
        }

        libraryAsset->AddClip(clip);
    }

    if (!file)
    {
        LOG_ERROR("Corrupt animation library data: {}", assetPath.string());
        return nullptr;
    }

    return libraryAsset;
}