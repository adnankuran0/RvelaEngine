#include "rvelapch.h"
#include "MeshComponent.h"

using namespace rv;

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

json MeshComponent::Serialize() const
{
    json j;
    j["mesh"] = meshUUID.ToString();
    return j;
}

void MeshComponent::Deserialize(const json& j)
{
    meshUUID = AssetUUID::FromString(j.at("mesh").get<std::string>());
    Load(meshUUID);
}
