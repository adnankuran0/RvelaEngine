#include "rvelapch.h"
#include "ScriptComponent.h"

using namespace rv;

json ScriptComponent::Serialize() const
{
    json j;
    j["scriptAssetUUID"] = scriptAssetUUID.ToString();
    return j;
}
void ScriptComponent::Deserialize(const json& j)
{
    scriptAssetUUID = AssetUUID::FromString(j.at("scriptAssetUUID").get<std::string>());
}
