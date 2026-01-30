#pragma once
#include "sol/sol.hpp"

struct ScriptComponent {
    std::string luaFile;
    sol::state* luaState = nullptr;
    sol::table luaInstance;

    sol::function OnCreate;
    sol::function OnUpdate;
    sol::function OnDestroy;
};
