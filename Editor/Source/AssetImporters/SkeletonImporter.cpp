#include "rvelapch.h"
#include "SkeletonImporter.h"
#include "Asset/AssetRegistry.h"
#include "Asset/AssetMeta.h"
#include "Asset/Types/SkeletonAsset.h"
#include "SkeletonSerializer.h"
#include "Core/Log.h"
#include "Math/RvelaMath.h"

using namespace rv;

void SkeletonImporter::CollectBoneSources( const aiScene* scene, std::unordered_map<std::string, BoneSource>& outBones) const
{
    for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
    {
        aiMesh* mesh = scene->mMeshes[m];
        for (uint32_t b = 0; b < mesh->mNumBones; ++b)
        {
            aiBone* bone = mesh->mBones[b];
            std::string name = bone->mName.C_Str();
            if (!outBones.contains(name))
                outBones[name] = BoneSource{ bone->mOffsetMatrix };
        }
    }
}

bool SkeletonImporter::SubtreeHasBone(const aiNode* node, const std::unordered_map<std::string, BoneSource>& bones) const
{
    if (bones.contains(node->mName.C_Str()))
        return true;

    for (uint32_t i = 0; i < node->mNumChildren; ++i)
        if (SubtreeHasBone(node->mChildren[i], bones))
            return true;

    return false;
}

void SkeletonImporter::BuildHierarchy(const aiNode* node, int32_t parentIndex, const aiMatrix4x4& parentGlobalTransform, 
    const std::unordered_map<std::string, BoneSource>& bones, const Ref<SkeletonAsset>& asset) const 
{
    aiMatrix4x4 globalTransform = parentGlobalTransform * node->mTransformation;

    std::string name = node->mName.C_Str();
    int32_t myIndex = (int32_t)asset->m_BoneNames.size();

    glm::mat4 localMat = math::ConvertToGlmMatrix(node->mTransformation);
    glm::vec3 pos, scale;
    glm::quat rot;
    math::DecomposeToQuat(localMat, scale, rot, pos);

    asset->m_BoneNames.push_back(name);
    asset->m_BoneNameToIndex[name] = myIndex;
    asset->m_ParentIndices.push_back(parentIndex);
    asset->m_BindPositions.push_back(pos);
    asset->m_BindRotations.push_back(rot);
    asset->m_BindScales.push_back(scale);

    auto it = bones.find(name);
    if (it != bones.end())
    {
        asset->m_InverseBindMatrices.push_back(
            math::ConvertToGlmMatrix(it->second.offsetMatrix));
    }
    else
    {
        aiMatrix4x4 inverseGlobal = globalTransform;
        inverseGlobal.Inverse();
        asset->m_InverseBindMatrices.push_back(math::ConvertToGlmMatrix(inverseGlobal));
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        if (SubtreeHasBone(node->mChildren[i], bones))
            BuildHierarchy(node->mChildren[i], myIndex, globalTransform, bones, asset);
    }
}

AssetUUID SkeletonImporter::ImportFromScene(const aiScene* scene, const std::filesystem::path& modelPath, AssetRegistry& registry)
{
    std::unordered_map<std::string, BoneSource> boneSources;
    CollectBoneSources(scene, boneSources);

    if (boneSources.empty())
        return AssetUUID::Invalid();

    if (!SubtreeHasBone(scene->mRootNode, boneSources))
        return AssetUUID::Invalid();

    auto skeletonPath = modelPath;
    skeletonPath.replace_extension(".rskeleton");

    AssetMeta meta = registry.GetOrCreateMeta(skeletonPath);
    if (meta.importerID.empty())
        meta.importerID = "SkeletonLoader";

    auto asset = CreateRef<SkeletonAsset>(meta.uuid);

    BuildHierarchy(scene->mRootNode, -1, aiMatrix4x4(), boneSources, asset);

    if (!SkeletonSerializer::Save(asset, skeletonPath))
        return AssetUUID::Invalid();

    registry.SaveMeta(skeletonPath, meta);
    return meta.uuid;
}