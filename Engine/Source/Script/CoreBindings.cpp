#include "rvelapch.h"
#include "CoreBindings.h"
#include "SceneBindings.h"
#include "sol/sol.hpp"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Scene/Components/UIComponents.h"
#include "Scene/Components/ParticleEmitterComponent.h"
#include "ComponentHandle.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetUUID.h"
#include <vector>

using namespace rv;

void LuaBindings::RegisterCoreTypes(sol::state& lua)
{
    lua.new_usertype<AssetHandle>("AssetHandle",
        sol::constructors<AssetHandle(), AssetHandle(const std::string&)>(),
        "ToString", &AssetHandle::ToString,
        "IsValid", &AssetHandle::IsValid,
        "IsEmpty", &AssetHandle::IsEmpty,
        sol::meta_function::to_string, &AssetHandle::ToString,
        sol::meta_function::equal_to, &AssetHandle::operator==,
        sol::meta_function::less_than, &AssetHandle::operator<
    );
    lua["AssetHandle"]["FromString"] = &AssetHandle::FromString;
    lua["AssetHandle"]["Invalid"] = &AssetHandle::Invalid;
    lua["AssetUUID"] = lua["AssetHandle"];

    lua["AssetManager"] = lua.create_table();
    lua["AssetManager"]["GetByPath"] = [](const std::string& path) -> AssetHandle {
        return AssetManager::Get().GetRegistry().GetUUID(path);
    };
    lua["AssetManager"]["GetPath"] = [](const AssetHandle& handle) -> std::string {
        return AssetManager::Get().GetRegistry().GetPath(handle).generic_string();
    };
    lua["AssetManager"]["Exists"] = [](const AssetHandle& handle) -> bool {
        return AssetManager::Get().GetRegistry().Exists(handle);
    };
    lua["AssetManager"]["GetHandle"] = [](sol::object obj) -> AssetHandle {
        if (obj.is<AssetHandle>()) {
            return obj.as<AssetHandle>();
        }
        if (obj.is<std::string>()) {
            std::string identifier = obj.as<std::string>();
            if (identifier.length() == 36 && identifier[8] == '-' && identifier[13] == '-' && identifier[18] == '-' && identifier[23] == '-')
            {
                AssetUUID parsed = AssetUUID::FromString(identifier);
                if (parsed.IsValid()) return parsed;
            }
            return AssetManager::Get().GetRegistry().GetUUID(identifier);
        }
        return AssetHandle::Invalid();
    };
    lua["Assets"] = lua["AssetManager"];

    lua.new_usertype<Entity>("Entity",
        "IsValid", &Entity::IsValid,
        "Destroy", [](Entity& self) {
            if (self.GetScene()) {
                self.GetScene()->QueueDestroyEntity(self);
            }
        },
        "name", sol::property(
            &Entity::GetName,
            &Entity::SetName
        ),
        "SetActive", &Entity::SetActive,
        "IsActive", &Entity::IsActive,
        "IsSelfActive", &Entity::IsSelfActive,
        "active", sol::property(&Entity::IsActive, &Entity::SetActive),
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
            if (type == "Transform") return e.HasComponent<TransformComponent>();
            else if (type == "Camera") return e.HasComponent<CameraComponent>();
            else if (type == "DirectionalLight") return e.HasComponent<DirectionalLightComponent>();
            else if (type == "PointLight") return e.HasComponent<PointLightComponent>();
            else if (type == "MeshRenderer") return e.HasComponent<MeshRendererComponent>();
            else if (type == "Material") return e.HasComponent<MaterialComponent>();
            else if (type == "Rigidbody") return e.HasComponent<RigidbodyComponent>();
            else if (type == "CharacterBody") return e.HasComponent<CharacterBodyComponent>();
            else if (type == "AudioEmitter") return e.HasComponent<AudioEmitterComponent>();
            else if (type == "Animator") return e.HasComponent<AnimatorComponent>();
            else if (type == "ParticleEmitter") return e.HasComponent<ParticleEmitterComponent>();
            else if (type == "RectTransform") return e.HasComponent<RectTransformComponent>();
            else if (type == "UICanvas") return e.HasComponent<UICanvasComponent>();
            else if (type == "UIImage") return e.HasComponent<UIImageComponent>();
            else if (type == "UIText") return e.HasComponent<UITextComponent>();
            else if (type == "UIButton") return e.HasComponent<UIButtonComponent>();
            else if (type == "UISlider") return e.HasComponent<UISliderComponent>();
            else if (type == "UIProgressBar") return e.HasComponent<UIProgressBarComponent>();
            else if (type == "UICheckbox") return e.HasComponent<UICheckboxComponent>();
            return false;
        },
        "GetComponent", [&](Entity& e, const std::string& type) -> sol::object {
            if (type == "Transform") {
                return sol::make_object(lua, ComponentHandle<TransformComponent>{ e });
            }
            else if (type == "Camera") {
                return sol::make_object(lua, ComponentHandle<CameraComponent>{ e });
            }
            else if (type == "DirectionalLight") {
                return sol::make_object(lua, ComponentHandle<DirectionalLightComponent>{ e });
            }
            else if (type == "PointLight") {
                return sol::make_object(lua, ComponentHandle<PointLightComponent>{ e });
            }
            else if (type == "MeshRenderer") {
                return sol::make_object(lua, ComponentHandle<MeshRendererComponent>{ e });
            }
            else if (type == "Material") {
                return sol::make_object(lua, ComponentHandle<MaterialComponent>{ e });
            }
            else if (type == "Rigidbody") {
                return sol::make_object(lua, ComponentHandle<RigidbodyComponent>{ e });
            }
            else if (type == "CharacterBody") {
                return sol::make_object(lua, ComponentHandle<CharacterBodyComponent>{ e });
            }
            else if (type == "AudioEmitter") {
                return sol::make_object(lua, ComponentHandle<AudioEmitterComponent>{ e });
            }
            else if (type == "Animator") {
                return sol::make_object(lua, ComponentHandle<AnimatorComponent>{ e });
            }
            else if (type == "ParticleEmitter") {
                return sol::make_object(lua, ComponentHandle<ParticleEmitterComponent>{ e });
            }
            else if (type == "RectTransform") {
                return sol::make_object(lua, ComponentHandle<RectTransformComponent>{ e });
            }
            else if (type == "UICanvas") {
                return sol::make_object(lua, ComponentHandle<UICanvasComponent>{ e });
            }
            else if (type == "UIImage") {
                return sol::make_object(lua, ComponentHandle<UIImageComponent>{ e });
            }
            else if (type == "UIText") {
                return sol::make_object(lua, ComponentHandle<UITextComponent>{ e });
            }
            else if (type == "UIButton") {
                return sol::make_object(lua, ComponentHandle<UIButtonComponent>{ e });
            }
            else if (type == "UISlider") {
                return sol::make_object(lua, ComponentHandle<UISliderComponent>{ e });
            }
            else if (type == "UIProgressBar") {
                return sol::make_object(lua, ComponentHandle<UIProgressBarComponent>{ e });
            }
            else if (type == "UICheckbox") {
                return sol::make_object(lua, ComponentHandle<UICheckboxComponent>{ e });
            }
            return sol::make_object(lua, sol::nil);
        },
        "AddComponent", [&](Entity& e, const std::string& typeName) -> sol::object {
            if (typeName == "PointLight") {
                if (!e.HasComponent<PointLightComponent>()) e.AddComponent<PointLightComponent>();
                return sol::make_object(lua, ComponentHandle<PointLightComponent>{ e });
            }
            else if (typeName == "DirectionalLight") {
                if (!e.HasComponent<DirectionalLightComponent>()) e.AddComponent<DirectionalLightComponent>();
                return sol::make_object(lua, ComponentHandle<DirectionalLightComponent>{ e });
            }
            else if (typeName == "Camera") {
                if (!e.HasComponent<CameraComponent>()) e.AddComponent<CameraComponent>();
                return sol::make_object(lua, ComponentHandle<CameraComponent>{ e });
            }
            else if (typeName == "Rigidbody") {
                if (!e.HasComponent<RigidbodyComponent>()) e.AddComponent<RigidbodyComponent>();
                return sol::make_object(lua, ComponentHandle<RigidbodyComponent>{ e });
            }
            else if (typeName == "AudioEmitter") {
                if (!e.HasComponent<AudioEmitterComponent>()) e.AddComponent<AudioEmitterComponent>();
                return sol::make_object(lua, ComponentHandle<AudioEmitterComponent>{ e });
            }
            else if (typeName == "Animator") {
                if (!e.HasComponent<AnimatorComponent>()) e.AddComponent<AnimatorComponent>();
                return sol::make_object(lua, ComponentHandle<AnimatorComponent>{ e });
            }
            else if (typeName == "RectTransform") {
                if (!e.HasComponent<RectTransformComponent>()) e.AddComponent<RectTransformComponent>();
                return sol::make_object(lua, ComponentHandle<RectTransformComponent>{ e });
            }
            else if (typeName == "UICanvas") {
                if (!e.HasComponent<UICanvasComponent>()) e.AddComponent<UICanvasComponent>();
                return sol::make_object(lua, ComponentHandle<UICanvasComponent>{ e });
            }
            else if (typeName == "UIImage") {
                if (!e.HasComponent<UIImageComponent>()) e.AddComponent<UIImageComponent>();
                return sol::make_object(lua, ComponentHandle<UIImageComponent>{ e });
            }
            else if (typeName == "UIText") {
                if (!e.HasComponent<UITextComponent>()) e.AddComponent<UITextComponent>();
                return sol::make_object(lua, ComponentHandle<UITextComponent>{ e });
            }
            else if (typeName == "UIButton") {
                if (!e.HasComponent<UIButtonComponent>()) e.AddComponent<UIButtonComponent>();
                return sol::make_object(lua, ComponentHandle<UIButtonComponent>{ e });
            }
            else if (typeName == "UISlider") {
                if (!e.HasComponent<UISliderComponent>()) e.AddComponent<UISliderComponent>();
                return sol::make_object(lua, ComponentHandle<UISliderComponent>{ e });
            }
            else if (typeName == "UIProgressBar") {
                if (!e.HasComponent<UIProgressBarComponent>()) e.AddComponent<UIProgressBarComponent>();
                return sol::make_object(lua, ComponentHandle<UIProgressBarComponent>{ e });
            }
            else if (typeName == "UICheckbox") {
                if (!e.HasComponent<UICheckboxComponent>()) e.AddComponent<UICheckboxComponent>();
                return sol::make_object(lua, ComponentHandle<UICheckboxComponent>{ e });
            }
            return sol::nil;
        }
    );
}
