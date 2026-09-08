#include "rvelapch.h"
#include "Scene/Components/SkeletalMeshComponent.h"
#include "Asset/AssetManager.h"
#include "Core/Log.h"

using namespace rv;

void SkeletalMeshComponent::Load(const AssetUUID& uuid)
{
    if (!uuid.IsValid())
    {
        LOG_WARN("SkeletalMesh UUID is not valid!");
        return;
    }
    meshUUID = uuid;
    mesh.Reset();
    mesh = AssetManager::Get().GetAsset<SkeletalMeshAsset>(meshUUID);
}

json SkeletalMeshComponent::Serialize() const
{
    json j;
    j["skeletalMesh"] = meshUUID.ToString();
    return j;
}

void SkeletalMeshComponent::Deserialize(const json& j)
{
    if (j.contains("skeletalMesh"))
    {
        meshUUID = AssetUUID::FromString(j.at("skeletalMesh").get<std::string>());
        Load(meshUUID);
    }
}