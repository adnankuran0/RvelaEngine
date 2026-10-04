#include "rvelapch.h"
#include "PrefabComponent.h"

using namespace rv;

json PrefabComponent::Serialize() const
{
    return prefabUUID.ToString(); 
}
void PrefabComponent::Deserialize(const json& j)
{
    if (j.is_string())
    {
        prefabUUID = AssetUUID::FromString(j.get<std::string>());
        m_Overrides = json::array();
    }
    else if (j.is_object())
    {
        prefabUUID = AssetUUID::FromString(j.value("uuid", ""));
        m_Overrides = j.value("overrides", json::array());
    }
}
