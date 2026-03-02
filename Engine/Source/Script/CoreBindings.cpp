#include "rvelapch.h"
#include "CoreBindings.h"
#include "sol/sol.hpp"
#include <Scene/Entity.h>

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
            return false;
        },
        "GetComponent", [&](Entity& e, const std::string& type) -> sol::object {
            if (type == "TransformComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<TransformComponent>()));
            }
            else if (type == "CameraComponent") {
                return sol::make_object(lua, std::ref(e.GetComponent<CameraComponent>()));
            }
            return sol::make_object(lua, sol::nil);
        }
    );

    lua.new_usertype<TransformComponent>("TransformComponent",
        "GetPosition", &TransformComponent::GetPosition,
        "SetPosition", &TransformComponent::SetPosition,
        "GetWorldPosition", &TransformComponent::GetWorldPosition,
        "Translate", &TransformComponent::Translate,
        "GetRotation", &TransformComponent::GetRotation,
        "GetWorldRotation", &TransformComponent::GetWorldRotation,
        "SetRotation", &TransformComponent::SetRotation,
        "GetEulerRotation", &TransformComponent::GetEulerRotation,
        "SetEulerRotation", &TransformComponent::SetEulerRotation,
        "GetScale", &TransformComponent::GetScale,
        "GetWorldScale", &TransformComponent::GetWorldScale,
        "SetScale", &TransformComponent::SetScale,
        "GetForward", &TransformComponent::GetForward,
        "GetUp", &TransformComponent::GetUp,
        "GetRight", &TransformComponent::GetRight,
        "LookAt", &TransformComponent::LookAt
    );

    lua.new_usertype<CameraComponent>("CameraComponent",
        "GetFOV", [](CameraComponent& c) { return (double)c.GetFOV(); },
        "SetFOV", &CameraComponent::SetFOV
    );
}