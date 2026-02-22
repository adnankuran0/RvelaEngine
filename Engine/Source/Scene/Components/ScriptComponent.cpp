#include "rvelapch.h"
#include "ScriptComponent.h"

using namespace rv;

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
