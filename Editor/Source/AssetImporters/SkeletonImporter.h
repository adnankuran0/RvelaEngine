#pragma once
#include "Asset/AssetUUID.h"
#include "Core/Ref.h"
#include <Assimp/scene.h>
#include <filesystem>
#include <unordered_map>
#include <string>

namespace rv {

class AssetRegistry;
class SkeletonAsset;

class SkeletonImporter
{
public:
    AssetUUID ImportFromScene(const aiScene* scene, const std::filesystem::path& modelPath, AssetRegistry& registry);

private:
    struct BoneSource { aiMatrix4x4 offsetMatrix; };

    void CollectBoneSources(const aiScene* scene, std::unordered_map<std::string, BoneSource>& outBones) const;
    bool SubtreeHasBone(const aiNode* node, const std::unordered_map<std::string, BoneSource>& bones) const;
    void BuildHierarchy(const aiNode* node, int32_t parentIndex, const aiMatrix4x4& parentGlobalTransform, 
        const std::unordered_map<std::string, BoneSource>& bones, const Ref<SkeletonAsset>& asset) const;
};

}