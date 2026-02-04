#include "rvelapch.h"
#include "ScriptComponent.h"

json ScriptComponent::Serialize() const
{
    json j;
    j["luaFile"] = luaFile;
    return j;
}
void ScriptComponent::Deserialize(const json& j)
{
    luaFile = j.at("luaFile").get<std::string>();
}
