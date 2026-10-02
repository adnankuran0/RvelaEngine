#include "rvelapch.h"
#include "CoreBindings.h"
#include "sol/sol.hpp"
#include "Scene/Entity.h"
#include "Scene/Components/UIComponents.h"
#include "ComponentHandle.h"
#include <vector>

using namespace rv;

void LuaBindings::RegisterCoreTypes(sol::state& lua)
{
    lua.new_usertype<Entity>("Entity",
        "IsValid", &Entity::IsValid,
        "name", sol::property(
            &Entity::GetName,
            &Entity::SetName
        ),
        "GetUUID", &Entity::GetUUID,
        "SetParent", [](Entity& self, sol::optional<Entity> parent) {
            if (parent.has_value() && parent.value()) {
                self.SetParent(parent.value());
            }
            else {
                self.SetParent(Entity{ entt::null, nullptr });
            }
        },
        "GetParent", [](Entity& self) -> sol::optional<Entity> {
            Entity parent = self.GetParent(self);
            if (parent) {
                return parent;
            }
            return sol::nullopt;
        },
        "FindChild", [](Entity& self, const std::string& childName) -> sol::optional<Entity> {
            Entity child = self.FindChild(childName);
            if (child) return child;
            return sol::nullopt;
        },
        "GetScriptType", [](Entity& e) -> std::string {
            if (!e.HasComponent<ScriptComponent>()) return "";
            auto& sc = e.GetComponent<ScriptComponent>();
            if (sc.luaInstance.valid() && sc.luaInstance["className"].valid()) {
                return sc.luaInstance["className"];
            }
            return "";
        },
        "HasMethod", [](Entity& e, const std::string& methodName) -> bool {
            if (!e.HasComponent<ScriptComponent>()) return false;
            auto& sc = e.GetComponent<ScriptComponent>();
            return sc.luaInstance.valid() && sc.luaInstance[methodName].valid();
        },
        "CallMethod", [](Entity& e, const std::string& methodName, sol::variadic_args va) -> sol::object {
            if (!e.HasComponent<ScriptComponent>()) return sol::nil;
            auto& sc = e.GetComponent<ScriptComponent>();

            if (sc.luaInstance.valid() && sc.luaInstance[methodName].valid()) {
                sol::protected_function func = sc.luaInstance[methodName];

                std::vector<sol::object> args;
                for (auto arg : va) {
                    args.push_back(arg.get<sol::object>());
                }

                sol::protected_function_result result = func(sc.luaInstance, sol::as_args(args));

                if (result.valid()) {
                    return result;
                }
                else {
                    sol::error err = result;
                    LOG_ERROR("Lua CallMethod error [{}]: {}", methodName, err.what());
                }
            }
            return sol::nil;
        },
        "HasComponent", [&](Entity& e, const std::string& type) {
            if (type == "TransformComponent") return e.HasComponent<TransformComponent>();
            else if (type == "CameraComponent") return e.HasComponent<CameraComponent>();
            else if (type == "DirectionalLightComponent") return e.HasComponent<DirectionalLightComponent>();
            else if (type == "PointLightComponent") return e.HasComponent<PointLightComponent>();
            else if (type == "MeshRendererComponent") return e.HasComponent<MeshRendererComponent>();
            else if (type == "MaterialComponent") return e.HasComponent<MaterialComponent>();
            else if (type == "RigidbodyComponent") return e.HasComponent<RigidbodyComponent>();
            else if (type == "CharacterBodyComponent") return e.HasComponent<CharacterBodyComponent>();
            else if (type == "AudioEmitterComponent") return e.HasComponent<AudioEmitterComponent>();
            else if (type == "AnimatorComponent") return e.HasComponent<AnimatorComponent>();
            else if (type == "RectTransformComponent") return e.HasComponent<RectTransformComponent>();
            else if (type == "UICanvasComponent") return e.HasComponent<UICanvasComponent>();
            else if (type == "UIImageComponent") return e.HasComponent<UIImageComponent>();
            else if (type == "UITextComponent") return e.HasComponent<UITextComponent>();
            else if (type == "UIButtonComponent") return e.HasComponent<UIButtonComponent>();
            else if (type == "UISliderComponent") return e.HasComponent<UISliderComponent>();
            else if (type == "UIProgressBarComponent") return e.HasComponent<UIProgressBarComponent>();
            else if (type == "UICheckboxComponent") return e.HasComponent<UICheckboxComponent>();
            return false;
        },
        "GetComponent", [&](Entity& e, const std::string& type) -> sol::object {
            if (type == "TransformComponent") {
                return sol::make_object(lua, ComponentHandle<TransformComponent>{ e });
            }
            else if (type == "CameraComponent") {
                return sol::make_object(lua, ComponentHandle<CameraComponent>{ e });
            }
            else if (type == "DirectionalLightComponent") {
                return sol::make_object(lua, ComponentHandle<DirectionalLightComponent>{ e });
            }
            else if (type == "PointLightComponent") {
                return sol::make_object(lua, ComponentHandle<PointLightComponent>{ e });
            }
            else if (type == "MeshRendererComponent") {
                return sol::make_object(lua, ComponentHandle<MeshRendererComponent>{ e });
            }
            else if (type == "MaterialComponent") {
                return sol::make_object(lua, ComponentHandle<MaterialComponent>{ e });
            }
            else if (type == "RigidbodyComponent") {
                return sol::make_object(lua, ComponentHandle<RigidbodyComponent>{ e });
            }
            else if (type == "CharacterBodyComponent") {
                return sol::make_object(lua, ComponentHandle<CharacterBodyComponent>{ e });
            }
            else if (type == "AudioEmitterComponent") {
                return sol::make_object(lua, ComponentHandle<AudioEmitterComponent>{ e });
            }
            else if (type == "AnimatorComponent") {
                return sol::make_object(lua, ComponentHandle<AnimatorComponent>{ e });
            }
            else if (type == "RectTransformComponent") {
                return sol::make_object(lua, ComponentHandle<RectTransformComponent>{ e });
            }
            else if (type == "UICanvasComponent") {
                return sol::make_object(lua, ComponentHandle<UICanvasComponent>{ e });
            }
            else if (type == "UIImageComponent") {
                return sol::make_object(lua, ComponentHandle<UIImageComponent>{ e });
            }
            else if (type == "UITextComponent") {
                return sol::make_object(lua, ComponentHandle<UITextComponent>{ e });
            }
            else if (type == "UIButtonComponent") {
                return sol::make_object(lua, ComponentHandle<UIButtonComponent>{ e });
            }
            else if (type == "UISliderComponent") {
                return sol::make_object(lua, ComponentHandle<UISliderComponent>{ e });
            }
            else if (type == "UIProgressBarComponent") {
                return sol::make_object(lua, ComponentHandle<UIProgressBarComponent>{ e });
            }
            else if (type == "UICheckboxComponent") {
                return sol::make_object(lua, ComponentHandle<UICheckboxComponent>{ e });
            }
            return sol::make_object(lua, sol::nil);
        },
        "AddComponent", [&](Entity& e, const std::string& typeName) -> sol::object {
            if (typeName == "PointLightComponent")
                return sol::make_object(lua, ComponentHandle<PointLightComponent>{ e });
            else if (typeName == "DirectionalLightComponent")
                return sol::make_object(lua, ComponentHandle<DirectionalLightComponent>{ e });
            else if (typeName == "CameraComponent")
                return sol::make_object(lua, ComponentHandle<CameraComponent>{ e });
            else if (typeName == "RigidbodyComponent")
                return sol::make_object(lua, ComponentHandle<RigidbodyComponent>{ e });
            else if (typeName == "AudioEmitterComponent")
                return sol::make_object(lua, ComponentHandle<AudioEmitterComponent>{ e });
            else if (typeName == "AnimatorComponent")
                return sol::make_object(lua, ComponentHandle<AnimatorComponent>{ e });
            else if (typeName == "RectTransformComponent")
                return sol::make_object(lua, ComponentHandle<RectTransformComponent>{ e });
            else if (typeName == "UICanvasComponent")
                return sol::make_object(lua, ComponentHandle<UICanvasComponent>{ e });
            else if (typeName == "UIImageComponent")
                return sol::make_object(lua, ComponentHandle<UIImageComponent>{ e });
            else if (typeName == "UITextComponent")
                return sol::make_object(lua, ComponentHandle<UITextComponent>{ e });
            else if (typeName == "UIButtonComponent")
                return sol::make_object(lua, ComponentHandle<UIButtonComponent>{ e });
            else if (typeName == "UISliderComponent")
                return sol::make_object(lua, ComponentHandle<UISliderComponent>{ e });
            else if (typeName == "UIProgressBarComponent")
                return sol::make_object(lua, ComponentHandle<UIProgressBarComponent>{ e });
            else if (typeName == "UICheckboxComponent") {
                if (!e.HasComponent<UICheckboxComponent>()) e.AddComponent<UICheckboxComponent>();
                return sol::make_object(lua, ComponentHandle<UICheckboxComponent>{ e });
            }
            return sol::nil;
        }
    );
}