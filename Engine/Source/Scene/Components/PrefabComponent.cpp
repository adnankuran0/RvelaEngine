#include "rvelapch.h"
#include "PrefabComponent.h"

namespace rv {

json PrefabComponent::Serialize() const
{
    return prefabUUID.ToString(); 
}
void PrefabComponent::Deserialize(const json& j)
{
    prefabUUID = AssetUUID::FromString(j.get<std::string>());
}

}