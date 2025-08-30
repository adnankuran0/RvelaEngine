#include "rvelapch.h"
#include "MeshComponent.h"

void MeshComponent::Load(const AssetUUID& uuid)
{
    if (!uuid.IsValid())
    {
        LOG_WARN("Material UUID is not valid!");
        return;
    }
    meshUUID = uuid;
    mesh.Reset();
    mesh = AssetRegistry::GetAsset<MeshAsset>(meshUUID);
}

std::string MeshComponent::Serialize() const
{
    json j;
    j["mesh"] = meshUUID.ToString();
    return j.dump(4);
}

void MeshComponent::Deserialize(const std::string& str)
{
    json j = json::parse(str);
    meshUUID = AssetUUID::FromString(j["mesh"]);
    Load(meshUUID);
}