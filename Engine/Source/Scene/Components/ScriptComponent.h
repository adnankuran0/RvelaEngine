#pragma once
#include "sol/sol.hpp"
#include "json.hpp"

using json = nlohmann::json;

struct ScriptComponent {
    std::string luaFile;
    sol::state* luaState = nullptr;
    sol::table luaInstance;

    sol::function OnCreate;
    sol::function OnUpdate;
    sol::function OnDestroy;

    json Serialize() const;
    void Deserialize(const json& j);
};
