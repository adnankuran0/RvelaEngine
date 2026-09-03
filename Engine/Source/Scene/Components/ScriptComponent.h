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

    sol::function OnCreate;
    sol::function OnUpdate;
    sol::function OnFixedUpdate;
    sol::function OnLateUpdate;
    sol::function OnDestroy;

    sol::function OnCollisionEnter;
    sol::function OnCollisionStay;
    sol::function OnCollisionExit;

    sol::function OnAnimationEvent;
    sol::function OnAnimationStarted;
    sol::function OnAnimationFinished;
    sol::function OnAnimationLooped;

    sol::function OnAudioFinished;

    json Serialize() const;
    void Deserialize(const json& j);
};

}