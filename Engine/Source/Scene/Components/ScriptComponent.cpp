#include "rvelapch.h"
#include "ScriptComponent.h"

using namespace rv;

json ScriptComponent::Serialize() const
{
    json j;
    j["scriptAssetUUID"] = scriptAssetUUID.ToString();
    j["propertyValues"] = propertyValues;
    return j;
}
void ScriptComponent::Deserialize(const json& j)
{
    if (j.contains("scriptAssetUUID") && j["scriptAssetUUID"].is_string())
        scriptAssetUUID = AssetUUID::FromString(j.at("scriptAssetUUID").get<std::string>());
    if (j.contains("propertyValues") && j["propertyValues"].is_object())
        propertyValues = j["propertyValues"];
    else
        propertyValues = json::object();
}
