#include "rvelapch.h"
#include "Scene/Components/SkeletonComponent.h"
#include "Asset/AssetManager.h"
#include "Core/Log.h"

using namespace rv;

void SkeletonComponent::Load(const AssetUUID& uuid)
{
    if (!uuid.IsValid())
    {
        LOG_WARN("Skeleton UUID is not valid!");
        return;
    }

    skeletonUUID = uuid;
    skeleton.Reset();
    skeleton = AssetManager::Get().GetAsset<SkeletonAsset>(skeletonUUID);

    InitFromAsset();
}

void SkeletonComponent::InitFromAsset()
{
    if (!skeleton || !skeleton->IsValid())
    {
        isInitialized = false;
        return;
    }

    uint32_t boneCount = skeleton->GetBoneCount();

    localPositions.resize(boneCount);
    localRotations.resize(boneCount);
    localScales.resize(boneCount);

    modelSpaceMatrices.resize(boneCount, glm::mat4(1.0f));
    skinningPalette.resize(boneCount, glm::mat4(1.0f));

    for (uint32_t i = 0; i < boneCount; ++i)
    {
        localPositions[i] = skeleton->GetBindPosition(i);
        localRotations[i] = skeleton->GetBindRotation(i);
        localScales[i] = skeleton->GetBindScale(i);
    }

    isInitialized = true;
}

json SkeletonComponent::Serialize() const
{
    json j;
    j["skeleton"] = skeletonUUID.ToString();
    return j;
}

void SkeletonComponent::Deserialize(const json& j)
{
    if (j.contains("skeleton"))
    {
        skeletonUUID = AssetUUID::FromString(j.at("skeleton").get<std::string>());
        Load(skeletonUUID);
    }
}