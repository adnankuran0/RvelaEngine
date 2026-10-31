#include "rvelapch.h"
#include "AnimationLibraryLoader.h"
#include "Asset/Types/AnimationLibraryAsset.h"
#include "Animation/BonePropertyTrack.h"
#include "Animation/EaseType.h"
#include "Asset/CacheTypes/AnimationLibraryHeader.h"
#include "Core/Log.h"
#include "Asset/AssetMeta.h"
#include <fstream>
#include <vector>
#include <cstring>

using namespace rv;
using namespace rv::Animation;

namespace {

struct BufferReader
{
    const uint8_t* data;
    size_t size;
    size_t offset = 0;

    bool Read(void* dest, size_t bytes)
    {
        if (offset + bytes > size) return false;
        std::memcpy(dest, data + offset, bytes);
        offset += bytes;
        return true;
    }

    std::string ReadString()
    {
        uint32_t len = 0;
        if (!Read(&len, sizeof(len)) || len == 0) return "";
        if (offset + len > size) return "";
        std::string str(reinterpret_cast<const char*>(data + offset), len);
        offset += len;
        return str;
    }
};

}

Ref<Asset> AnimationLibraryLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        LOG_ERROR("Cannot open animation library: {}", assetPath.string());
        return nullptr;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize <= 0)
    {
        LOG_ERROR("Empty animation library file: {}", assetPath.string());
        return nullptr;
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
    {
        LOG_ERROR("Failed to read animation library data: {}", assetPath.string());
        return nullptr;
    }

    return LoadFromMemory(buffer.data(), buffer.size(), meta);
}

Ref<Asset> AnimationLibraryLoader::LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta)
{
    if (!data || size < sizeof(AnimationLibraryHeader))
    {
        LOG_ERROR("Invalid buffer size for animation library UUID: {}", meta.uuid.ToString());
        return nullptr;
    }

    BufferReader reader{ data, size, 0 };

    AnimationLibraryHeader header{};
    if (!reader.Read(&header, sizeof(header)))
    {
        LOG_ERROR("Failed to read animation library header for UUID: {}", meta.uuid.ToString());
        return nullptr;
    }

    if (header.magic != MAGIC_ANIM_LIB)
    {
        LOG_ERROR("Invalid animation library magic for UUID: {}", meta.uuid.ToString());
        return nullptr;
    }

    if (header.version != ANIM_LIB_VERSION)
    {
        LOG_ERROR("Animation library version mismatch (got {}, expected {}) for UUID: {}",
            header.version, ANIM_LIB_VERSION, meta.uuid.ToString());
        return nullptr;
    }

    auto libraryAsset = CreateRef<AnimationLibraryAsset>(meta.uuid);

    for (uint32_t c = 0; c < header.clipCount; ++c)
    {
        std::string clipName = reader.ReadString();
        auto clip = std::make_shared<AnimationClip>(clipName);

        if (!reader.Read(&clip->duration, sizeof(clip->duration))) return nullptr;

        int32_t loopMode = 0;
        if (!reader.Read(&loopMode, sizeof(loopMode))) return nullptr;
        clip->loopMode = static_cast<LoopMode>(loopMode);

        // Events
        uint32_t eventCount = 0;
        if (!reader.Read(&eventCount, sizeof(eventCount))) return nullptr;
        for (uint32_t e = 0; e < eventCount; ++e)
        {
            float time = 0.0f;
            if (!reader.Read(&time, sizeof(time))) return nullptr;
            std::string evName = reader.ReadString();
            std::string evParam = reader.ReadString();
            clip->AddEvent(time, evName, evParam);
        }

        // Property Tracks
        uint32_t trackCount = 0;
        if (!reader.Read(&trackCount, sizeof(trackCount))) return nullptr;

        for (uint32_t t = 0; t < trackCount; ++t)
        {
            std::string targetPath = reader.ReadString();
            std::string propName = reader.ReadString();

            uint32_t propTypeVal = 0;
            if (!reader.Read(&propTypeVal, sizeof(propTypeVal))) return nullptr;
            auto propType = static_cast<PropertyType>(propTypeVal);

            if (propType == PropertyType::BoneVec3)
            {
                uint32_t transType = 0;
                if (!reader.Read(&transType, sizeof(transType))) return nullptr;

                uint32_t kfCount = 0;
                if (!reader.Read(&kfCount, sizeof(kfCount))) return nullptr;

                auto bTrack = std::make_shared<BonePropertyTrack<glm::vec3>>();
                bTrack->targetPath = targetPath;
                bTrack->propertyName = propName;
                bTrack->transformType = static_cast<BoneTransformType>(transType);
                bTrack->track.keyframes.resize(kfCount);
                if (!reader.Read(bTrack->track.keyframes.data(), kfCount * sizeof(Keyframe<glm::vec3>))) return nullptr;
                clip->propertyTracks.push_back(bTrack);
            }
            else if (propType == PropertyType::BoneQuat)
            {
                uint32_t transType = 0;
                if (!reader.Read(&transType, sizeof(transType))) return nullptr;

                uint32_t kfCount = 0;
                if (!reader.Read(&kfCount, sizeof(kfCount))) return nullptr;

                auto bTrack = std::make_shared<BonePropertyTrack<glm::quat>>();
                bTrack->targetPath = targetPath;
                bTrack->propertyName = propName;
                bTrack->transformType = static_cast<BoneTransformType>(transType);
                bTrack->track.keyframes.resize(kfCount);
                if (!reader.Read(bTrack->track.keyframes.data(), kfCount * sizeof(Keyframe<glm::quat>))) return nullptr;
                clip->propertyTracks.push_back(bTrack);
            }
            else if (propType == PropertyType::Float)
            {
                uint32_t kfCount = 0;
                if (!reader.Read(&kfCount, sizeof(kfCount))) return nullptr;
                auto tTrack = std::make_shared<TypedPropertyTrack<float>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                if (!reader.Read(tTrack->track.keyframes.data(), kfCount * sizeof(Keyframe<float>))) return nullptr;
                clip->propertyTracks.push_back(tTrack);
            }
            else if (propType == PropertyType::Vec3)
            {
                uint32_t kfCount = 0;
                if (!reader.Read(&kfCount, sizeof(kfCount))) return nullptr;
                auto tTrack = std::make_shared<TypedPropertyTrack<glm::vec3>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                if (!reader.Read(tTrack->track.keyframes.data(), kfCount * sizeof(Keyframe<glm::vec3>))) return nullptr;
                clip->propertyTracks.push_back(tTrack);
            }
            else if (propType == PropertyType::Vec4)
            {
                uint32_t kfCount = 0;
                if (!reader.Read(&kfCount, sizeof(kfCount))) return nullptr;
                auto tTrack = std::make_shared<TypedPropertyTrack<glm::vec4>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                if (!reader.Read(tTrack->track.keyframes.data(), kfCount * sizeof(Keyframe<glm::vec4>))) return nullptr;
                clip->propertyTracks.push_back(tTrack);
            }
            else if (propType == PropertyType::Quat)
            {
                uint32_t kfCount = 0;
                if (!reader.Read(&kfCount, sizeof(kfCount))) return nullptr;
                auto tTrack = std::make_shared<TypedPropertyTrack<glm::quat>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                if (!reader.Read(tTrack->track.keyframes.data(), kfCount * sizeof(Keyframe<glm::quat>))) return nullptr;
                clip->propertyTracks.push_back(tTrack);
            }
            else if (propType == PropertyType::Bool)
            {
                uint32_t kfCount = 0;
                if (!reader.Read(&kfCount, sizeof(kfCount))) return nullptr;
                auto tTrack = std::make_shared<TypedPropertyTrack<bool>>();
                tTrack->targetPath = targetPath;
                tTrack->propertyName = propName;
                tTrack->track.keyframes.resize(kfCount);
                if (!reader.Read(tTrack->track.keyframes.data(), kfCount * sizeof(Keyframe<bool>))) return nullptr;
                clip->propertyTracks.push_back(tTrack);
            }
        }

        libraryAsset->AddClip(clip);
    }

    return libraryAsset;
}
