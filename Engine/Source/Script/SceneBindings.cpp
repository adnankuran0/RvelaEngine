#include "rvelapch.h"
#include "SceneBindings.h"
#include "sol/sol.hpp"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetUUID.h"
#include <glm/gtc/quaternion.hpp>

using namespace rv;

Entity LuaBindings::InstantiatePrefabHelper(Scene& scene, sol::object prefabObj,
    sol::optional<sol::object> posObj,
    sol::optional<sol::object> rotObj,
    sol::optional<sol::object> parentObj)
{
    AssetUUID prefabUUID = AssetUUID::Invalid();

    if (prefabObj.is<AssetHandle>())
    {
        prefabUUID = prefabObj.as<AssetHandle>();
    }
    else if (prefabObj.is<AssetHandle*>())
    {
        AssetHandle* ptr = prefabObj.as<AssetHandle*>();
        if (ptr) prefabUUID = *ptr;
    }
    else if (prefabObj.is<std::string>() || prefabObj.get_type() == sol::type::string)
    {
        std::string str = prefabObj.as<std::string>();
        if (str.length() == 36 && str[8] == '-' && str[13] == '-' && str[18] == '-' && str[23] == '-')
        {
            prefabUUID = AssetUUID::FromString(str);
        }
        if (!prefabUUID.IsValid())
        {
            prefabUUID = AssetManager::Get().GetRegistry().GetUUID(str);
        }
    }

    if (!prefabUUID.IsValid())
    {
        std::string typeName = sol::type_name(prefabObj.lua_state(), prefabObj.get_type());
        std::string desc = (prefabObj.is<std::string>() || prefabObj.get_type() == sol::type::string) ? prefabObj.as<std::string>() : typeName;
        LOG_ERROR("[Scene:Instantiate] Failed to resolve prefab from argument: '{}' (type: {})", desc, typeName);
        return Entity{};
    }

    glm::vec3 position(0.0f);
    if (posObj.has_value() && posObj.value().is<glm::vec3>())
    {
        position = posObj.value().as<glm::vec3>();
    }

    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    if (rotObj.has_value())
    {
        if (rotObj.value().is<glm::quat>())
        {
            rotation = rotObj.value().as<glm::quat>();
        }
        else if (rotObj.value().is<glm::vec3>())
        {
            glm::vec3 euler = rotObj.value().as<glm::vec3>();
            rotation = glm::quat(glm::radians(euler));
        }
    }

    entt::entity parentHandle = entt::null;
    if (parentObj.has_value())
    {
        if (parentObj.value().is<Entity>())
        {
            parentHandle = parentObj.value().as<Entity>().GetHandle();
        }
    }

    return scene.Instantiate(prefabUUID, position, rotation, parentHandle);
}

void LuaBindings::RegisterSceneAPI(sol::state& lua)
{
    lua["Scene"] = lua.create_table();

    lua.new_usertype<Scene>("Scene",
        "CreateEntity", &Scene::CreateEntity,
        "DestroyEntity", [](Scene& scene, Entity& e) { scene.QueueDestroyEntity(e); },
        "FindEntityByName", &Scene::GetEntityByName,
        "Instantiate", [](Scene& scene, sol::object prefabObj, sol::optional<sol::object> posObj, sol::optional<sol::object> rotObj, sol::optional<sol::object> parentObj) {
            return InstantiatePrefabHelper(scene, prefabObj, posObj, rotObj, parentObj);
        }
    );
}