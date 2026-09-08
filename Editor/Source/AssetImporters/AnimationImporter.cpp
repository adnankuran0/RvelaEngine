#include "rvelapch.h"
#include "AnimationImporter.h"
#include "Asset/Types/AnimationLibraryAsset.h"
#include "AnimationLibrarySerializer.h"
#include "Animation/BonePropertyTrack.h"
#include "Asset/AssetRegistry.h"
#include "Core/Log.h"
#include <Assimp/scene.h>

using namespace rv;

static bool IsConstantTrack(const aiVectorKey* keys, unsigned int count, float epsilon = 0.0001f)
{
    if (count <= 1) return true;
    const auto& first = keys[0].mValue;
    for (unsigned int i = 1; i < count; ++i)
    {
        if (std::abs(keys[i].mValue.x - first.x) > epsilon ||
            std::abs(keys[i].mValue.y - first.y) > epsilon ||
            std::abs(keys[i].mValue.z - first.z) > epsilon)
            return false;
    }
    return true;
}

static bool IsConstantTrack(const aiQuatKey* keys, unsigned int count, float epsilon = 0.0001f)
{
    if (count <= 1) return true;
    const auto& first = keys[0].mValue;
    for (unsigned int i = 1; i < count; ++i)
    {
        if (std::abs(keys[i].mValue.x - first.x) > epsilon ||
            std::abs(keys[i].mValue.y - first.y) > epsilon ||
            std::abs(keys[i].mValue.z - first.z) > epsilon ||
            std::abs(keys[i].mValue.w - first.w) > epsilon)
            return false;
    }
    return true;
}

AssetUUID AnimationImporter::ImportFromScene(
    const aiScene* scene,
    const std::filesystem::path& modelPath,
    AssetRegistry& registry,
    const std::unordered_map<std::string, int32_t>& boneNameToIndex)
{
    if (!scene || scene->mNumAnimations == 0)
        return AssetUUID::Invalid();

    std::string animLibFileName = modelPath.stem().string() + ".ranimlib";
    std::filesystem::path animLibPath = modelPath.parent_path() / animLibFileName;

    AssetMeta meta = registry.GetOrCreateMeta(animLibPath);
    meta.importerID = "AnimationLibraryLoader";

    auto libraryAsset = CreateRef<AnimationLibraryAsset>(meta.uuid);

    for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
    {
        aiAnimation* anim = scene->mAnimations[i];
        std::string clipName = anim->mName.C_Str();
        if (clipName.empty()) clipName = "Clip_" + std::to_string(i);

        auto clip = std::make_shared<Animation::AnimationClip>(clipName);
        float ticksPerSecond = anim->mTicksPerSecond != 0.0 ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;
        clip->duration = static_cast<float>(anim->mDuration / ticksPerSecond);

        for (unsigned int c = 0; c < anim->mNumChannels; ++c)
        {
            aiNodeAnim* channel = anim->mChannels[c];
            std::string boneName = channel->mNodeName.C_Str();

            if (!boneNameToIndex.contains(boneName))
                continue;

            // 1. Position Track
            if (channel->mNumPositionKeys > 0)
            {
                auto posTrack = std::make_shared<Animation::BonePropertyTrack<glm::vec3>>();
                posTrack->targetPath = boneName;
                posTrack->propertyName = "local_position";
                posTrack->transformType = Animation::BoneTransformType::Position;

                if (IsConstantTrack(channel->mPositionKeys, channel->mNumPositionKeys))
                {
                    auto& k = channel->mPositionKeys[0];
                    posTrack->track.AddKeyframe(0.0f, glm::vec3(k.mValue.x, k.mValue.y, k.mValue.z));
                }
                else
                {
                    for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k)
                    {
                        auto& key = channel->mPositionKeys[k];
                        float t = static_cast<float>(key.mTime / ticksPerSecond);
                        posTrack->track.AddKeyframe(t, glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z));
                    }
                }
                clip->propertyTracks.push_back(posTrack);
            }

            // 2. Rotation Track
            if (channel->mNumRotationKeys > 0)
            {
                auto rotTrack = std::make_shared<Animation::BonePropertyTrack<glm::quat>>();
                rotTrack->targetPath = boneName;
                rotTrack->propertyName = "local_rotation";
                rotTrack->transformType = Animation::BoneTransformType::Rotation;

                if (IsConstantTrack(channel->mRotationKeys, channel->mNumRotationKeys))
                {
                    auto& k = channel->mRotationKeys[0];
                    rotTrack->track.AddKeyframe(0.0f, glm::quat(k.mValue.w, k.mValue.x, k.mValue.y, k.mValue.z));
                }
                else
                {
                    for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k)
                    {
                        auto& key = channel->mRotationKeys[k];
                        float t = static_cast<float>(key.mTime / ticksPerSecond);
                        rotTrack->track.AddKeyframe(t, glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z));
                    }
                }
                clip->propertyTracks.push_back(rotTrack);
            }

            // 3. Scale Track
            if (channel->mNumScalingKeys > 0)
            {
                auto scaleTrack = std::make_shared<Animation::BonePropertyTrack<glm::vec3>>();
                scaleTrack->targetPath = boneName;
                scaleTrack->propertyName = "local_scale";
                scaleTrack->transformType = Animation::BoneTransformType::Scale;

                if (IsConstantTrack(channel->mScalingKeys, channel->mNumScalingKeys))
                {
                    auto& k = channel->mScalingKeys[0];
                    scaleTrack->track.AddKeyframe(0.0f, glm::vec3(k.mValue.x, k.mValue.y, k.mValue.z));
                }
                else
                {
                    for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k)
                    {
                        auto& key = channel->mScalingKeys[k];
                        float t = static_cast<float>(key.mTime / ticksPerSecond);
                        scaleTrack->track.AddKeyframe(t, glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z));
                    }
                }
                clip->propertyTracks.push_back(scaleTrack);
            }
        }

        libraryAsset->AddClip(clip);
    }

    if (AnimationLibrarySerializer::Save(libraryAsset, animLibPath))
    {
        registry.SaveMeta(animLibPath, meta);
        registry.RegisterSubAsset(meta.uuid, animLibPath, "AnimationLibraryLoader");
        return meta.uuid;
    }

    return AssetUUID::Invalid();
}