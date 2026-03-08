#include "rvelapch.h"
#include "CoreBindings.h"
#include "sol/sol.hpp"
#include "Scene/Entity.h"

using namespace rv;

void LuaBindings::RegisterCoreTypes(sol::state& lua)
{
    lua.new_usertype<Entity>("Entity",
        "GetName", &Entity::GetName,
        "SetName", &Entity::SetName,
        "GetUUID", &Entity::GetUUID,
        "HasComponent", [&](Entity& e, const std::string& type) {
            // TODO: RTTR
            if (type == "TransformComponent") return e.HasComponent<TransformComponent>();
            else if (type == "CameraComponent") return e.HasComponent<CameraComponent>();
            else if (type == "DirectionalLightComponent") return e.HasComponent<DirectionalLightComponent>();
            else if (type == "PointLightComponent") return e.HasComponent<PointLightComponent>();
            else if (type == "MeshRendererComponent") return e.HasComponent<MeshRendererComponent>();
            else if (type == "MaterialComponent") return e.HasComponent<MaterialComponent>();
            else if (type == "RigidbodyComponent") return e.HasComponent<RigidbodyComponent>();
            else if (type == "CharacterBodyComponent") return e.HasComponent<CharacterBodyComponent>();
            return false;
        },
        "GetComponent", [&](Entity& e, const std::string& type) -> sol::object {
            if (type == "TransformComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<TransformComponent>()));
            }
            else if (type == "CameraComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<CameraComponent>()));
            }
            else if (type == "DirectionalLightComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<DirectionalLightComponent>()));
            }
            else if (type == "PointLightComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<PointLightComponent>()));
            }
            else if (type == "MeshRendererComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<MeshRendererComponent>()));
            }
            else if (type == "MaterialComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<MaterialComponent>()));
            }
            else if (type == "RigidbodyComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<RigidbodyComponent>()));
            }
            else if (type == "CharacterBodyComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<CharacterBodyComponent>()));
            }
            return sol::make_object(lua, sol::nil);
        },
        "AddComponent", [&](Entity& e, const std::string& typeName) -> sol::object {
            if (typeName == "PointLightComponent")
                return sol::make_object(lua, &e.AddComponent<PointLightComponent>());
            else if (typeName == "DirectionalLightComponent")
                return sol::make_object(lua, &e.AddComponent<DirectionalLightComponent>());
            else if (typeName == "CameraComponent")
                return sol::make_object(lua, &e.AddComponent<CameraComponent>());

            return sol::nil;
        }
    );

   
}