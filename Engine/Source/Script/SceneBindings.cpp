#include "rvelapch.h"
#include "SceneBindings.h"
#include "sol/sol.hpp"
#include "Scene/Entity.h"

using namespace rv;

void LuaBindings::RegisterSceneAPI(sol::state& lua)
{
    lua["Scene"] = lua.create_table();

    lua.new_usertype<Scene>("Scene",
        "CreateEntity", &Scene::CreateEntity,
        "DestroyEntity", [](Scene& scene, Entity& e) { scene.DestroyEntity(e); },
        "FindEntityByName", &Scene::GetEntityByName
        
    );
}