#pragma once
#include "sol/sol.hpp"
#include "json.hpp"
#include "Asset/AssetUUID.h"

namespace rv {

using json = nlohmann::json;

struct ScriptComponent 
{
    AssetUUID scriptAssetUUID{};

    sol::state* luaState = nullptr;
    sol::table luaInstance;

    sol::protected_function OnCreate;
    sol::protected_function OnUpdate;
    sol::protected_function OnFixedUpdate;
    sol::protected_function OnLateUpdate;
    sol::protected_function OnDestroy;

    sol::protected_function OnCollisionEnter;
    sol::protected_function OnCollisionStay;
    sol::protected_function OnCollisionExit;

    sol::protected_function OnAnimationEvent;
    sol::protected_function OnAnimationStarted;
    sol::protected_function OnAnimationFinished;
    sol::protected_function OnAnimationLooped;

    sol::protected_function OnAudioFinished;

    json Serialize() const;
    void Deserialize(const json& j);
};

}