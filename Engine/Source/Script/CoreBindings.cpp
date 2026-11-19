#include "rvelapch.h"
#include "CoreBindings.h"
#include "SceneBindings.h"
#include "sol/sol.hpp"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Scene/Components/UIComponents.h"
#include "Scene/Components/ParticleEmitterComponent.h"
#include "Scene/Components/ColliderComponents.h"
#include "Scene/Components/PhysicsConstraintComponent.h"
#include "Scene/Components/NavMeshSurfaceComponent.h"
#include "Scene/Components/NavMeshAgentComponent.h"
#include "Scene/Components/NavMeshObstacleComponent.h"
#include "ComponentHandle.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetUUID.h"
#include "Core/Time.h"
#include "Core/Engine.h"
#include "Core/ProjectSettings.h"
#include "Utils/ProjectManager.h"
#include "Utils/FileUtils.h"
#include <vector>
#include <unordered_set>
#include <algorithm>

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
        "ConnectSignal", [](Entity& source, const std::string& signalName, Entity target, const std::string& methodName) -> uint64_t {
            if (!source.GetScene() || target.GetScene() != source.GetScene()) return 0;
            return source.GetScene()->GetScriptSystem().ConnectSignal(
                source.GetHandle(), signalName, target.GetHandle(), methodName);
        },
        "DisconnectSignal", [](Entity& source, uint64_t connectionId) -> bool {
            if (!source.GetScene()) return false;
            return source.GetScene()->GetScriptSystem().DisconnectSignal(source.GetHandle(), connectionId);
        },
        "HasComponent", [&](Entity& e, const std::string& type) {
            if (type == "Transform") return e.HasComponent<TransformComponent>();
            else if (type == "Camera") return e.HasComponent<CameraComponent>();
            else if (type == "DirectionalLight") return e.HasComponent<DirectionalLightComponent>();
            else if (type == "PointLight") return e.HasComponent<PointLightComponent>();
            else if (type == "SpotLight") return e.HasComponent<SpotLightComponent>();
            else if (type == "MeshRenderer") return e.HasComponent<MeshRendererComponent>();
            else if (type == "SkeletalMeshRenderer") return e.HasComponent<SkeletalMeshRendererComponent>();
            else if (type == "Material") return e.HasComponent<MaterialComponent>();
            else if (type == "Rigidbody") return e.HasComponent<RigidbodyComponent>();
            else if (type == "PhysicsConstraint") return e.HasComponent<PhysicsConstraintComponent>();
            else if (type == "NavMeshSurface") return e.HasComponent<NavMeshSurfaceComponent>();
            else if (type == "NavMeshAgent") return e.HasComponent<NavMeshAgentComponent>();
            else if (type == "NavMeshObstacle") return e.HasComponent<NavMeshObstacleComponent>();
            else if (type == "CharacterBody") return e.HasComponent<CharacterBodyComponent>();
            else if (type == "CapsuleCollider") return e.HasComponent<CapsuleColliderComponent>();
            else if (type == "BoxCollider") return e.HasComponent<BoxColliderComponent>();
            else if (type == "SphereCollider") return e.HasComponent<SphereColliderComponent>();
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
            else if (type == "UIPanel") return e.HasComponent<UIPanelComponent>();
            else if (type == "UILayoutGroup") return e.HasComponent<UILayoutGroupComponent>();
            else if (type == "UITextInput") return e.HasComponent<UITextInputComponent>();
            else if (type == "UIScrollRect") return e.HasComponent<UIScrollRectComponent>();
            else if (type == "UITabContainer") return e.HasComponent<UITabContainerComponent>();
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
            else if (type == "SpotLight") {
                return sol::make_object(lua, ComponentHandle<SpotLightComponent>{ e });
            }
            else if (type == "MeshRenderer") {
                return sol::make_object(lua, ComponentHandle<MeshRendererComponent>{ e });
            }
            else if (type == "SkeletalMeshRenderer") {
                return sol::make_object(lua, ComponentHandle<SkeletalMeshRendererComponent>{ e });
            }
            else if (type == "Material") {
                return sol::make_object(lua, ComponentHandle<MaterialComponent>{ e });
            }
            else if (type == "Rigidbody") {
                return sol::make_object(lua, ComponentHandle<RigidbodyComponent>{ e });
            }
            else if (type == "PhysicsConstraint") {
                return sol::make_object(lua, ComponentHandle<PhysicsConstraintComponent>{ e });
            }
            else if (type == "NavMeshSurface") {
                return sol::make_object(lua, ComponentHandle<NavMeshSurfaceComponent>{ e });
            }
            else if (type == "NavMeshAgent") {
                return sol::make_object(lua, ComponentHandle<NavMeshAgentComponent>{ e });
            }
            else if (type == "NavMeshObstacle") {
                return sol::make_object(lua, ComponentHandle<NavMeshObstacleComponent>{ e });
            }
            else if (type == "CharacterBody") {
                return sol::make_object(lua, ComponentHandle<CharacterBodyComponent>{ e });
            }
            else if (type == "CapsuleCollider") {
                return sol::make_object(lua, ComponentHandle<CapsuleColliderComponent>{ e });
            }
            else if (type == "BoxCollider") {
                return sol::make_object(lua, ComponentHandle<BoxColliderComponent>{ e });
            }
            else if (type == "SphereCollider") {
                return sol::make_object(lua, ComponentHandle<SphereColliderComponent>{ e });
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
            else if (type == "UIPanel") {
                return sol::make_object(lua, ComponentHandle<UIPanelComponent>{ e });
            }
            else if (type == "UILayoutGroup") {
                return sol::make_object(lua, ComponentHandle<UILayoutGroupComponent>{ e });
            }
            else if (type == "UITextInput") {
                return sol::make_object(lua, ComponentHandle<UITextInputComponent>{ e });
            }
            else if (type == "UIScrollRect") {
                return sol::make_object(lua, ComponentHandle<UIScrollRectComponent>{ e });
            }
            else if (type == "UITabContainer") {
                return sol::make_object(lua, ComponentHandle<UITabContainerComponent>{ e });
            }
            return sol::make_object(lua, sol::nil);
        },
        "AddComponent", [&](Entity& e, const std::string& typeName) -> sol::object {
            if (typeName == "PointLight") {
                if (!e.HasComponent<PointLightComponent>()) e.AddComponent<PointLightComponent>();
                return sol::make_object(lua, ComponentHandle<PointLightComponent>{ e });
            }
            else if (typeName == "SpotLight") {
                if (!e.HasComponent<SpotLightComponent>()) e.AddComponent<SpotLightComponent>();
                return sol::make_object(lua, ComponentHandle<SpotLightComponent>{ e });
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
            else if (typeName == "PhysicsConstraint") {
                if (!e.HasComponent<PhysicsConstraintComponent>()) e.AddComponent<PhysicsConstraintComponent>();
                return sol::make_object(lua, ComponentHandle<PhysicsConstraintComponent>{ e });
            }
            else if (typeName == "NavMeshSurface") {
                if (!e.HasComponent<NavMeshSurfaceComponent>()) e.AddComponent<NavMeshSurfaceComponent>();
                return sol::make_object(lua, ComponentHandle<NavMeshSurfaceComponent>{ e });
            }
            else if (typeName == "NavMeshAgent") {
                if (!e.HasComponent<NavMeshAgentComponent>()) e.AddComponent<NavMeshAgentComponent>();
                return sol::make_object(lua, ComponentHandle<NavMeshAgentComponent>{ e });
            }
            else if (typeName == "NavMeshObstacle") {
                if (!e.HasComponent<NavMeshObstacleComponent>()) e.AddComponent<NavMeshObstacleComponent>();
                return sol::make_object(lua, ComponentHandle<NavMeshObstacleComponent>{ e });
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
            else if (typeName == "UIPanel") {
                if (!e.HasComponent<UIPanelComponent>()) e.AddComponent<UIPanelComponent>();
                return sol::make_object(lua, ComponentHandle<UIPanelComponent>{ e });
            }
            else if (typeName == "UILayoutGroup") {
                if (!e.HasComponent<UILayoutGroupComponent>()) e.AddComponent<UILayoutGroupComponent>();
                return sol::make_object(lua, ComponentHandle<UILayoutGroupComponent>{ e });
            }
            else if (typeName == "UITextInput") {
                if (!e.HasComponent<UITextInputComponent>()) e.AddComponent<UITextInputComponent>();
                return sol::make_object(lua, ComponentHandle<UITextInputComponent>{ e });
            }
            else if (typeName == "UIScrollRect") {
                if (!e.HasComponent<UIScrollRectComponent>()) e.AddComponent<UIScrollRectComponent>();
                return sol::make_object(lua, ComponentHandle<UIScrollRectComponent>{ e });
            }
            else if (typeName == "UITabContainer") {
                if (!e.HasComponent<UITabContainerComponent>()) e.AddComponent<UITabContainerComponent>();
                return sol::make_object(lua, ComponentHandle<UITabContainerComponent>{ e });
            }
            return sol::nil;
        }
    );

    auto timeTable = lua.create_named_table("Time");
    timeTable["GetFPS"] = &Time::GetFPS;
    timeTable["GetDeltaTime"] = &Time::GetDeltaTime;
    timeTable["GetFixedDeltaTime"] = &Time::GetFixedDeltaTime;
    timeTable["SetFixedDeltaTime"] = &Time::SetFixedDeltaTime;
    timeTable["GetTimeScale"] = &Time::GetTimeScale;
    timeTable["SetTimeScale"] = &Time::SetTimeScale;
    timeTable["GetMaxFPS"] = &Time::GetMaxFPS;
    timeTable["SetMaxFPS"] = &Time::SetMaxFPS;

    sol::table timeMeta = lua.create_table();
    timeMeta[sol::meta_function::index] = [](sol::table t, const std::string& key) -> sol::object {
        if (key == "fps") return sol::make_object(t.lua_state(), Time::GetFPS());
        if (key == "deltaTime") return sol::make_object(t.lua_state(), Time::GetDeltaTime());
        if (key == "timeScale") return sol::make_object(t.lua_state(), Time::GetTimeScale());
        if (key == "time") return sol::make_object(t.lua_state(), static_cast<float>(Time::GetCurrentTime()));
        return sol::nil;
    };
    timeTable[sol::metatable_key] = timeMeta;

    auto engineTable = lua.create_named_table("Engine");
    engineTable["Quit"] = []() {
        if (Engine::Get()) {
            Engine::Get()->Quit();
        }
    };
    engineTable["Exit"] = engineTable["Quit"];

    auto appTable = lua.create_named_table("Application");
    appTable["Quit"] = engineTable["Quit"];
    appTable["Exit"] = engineTable["Quit"];
    appTable["SetMaxFPS"] = [](int fps) { Time::SetMaxFPS(fps); };
    appTable["GetMaxFPS"] = []() -> int { return Time::GetMaxFPS(); };

    engineTable["SetMaxFPS"] = appTable["SetMaxFPS"];
    engineTable["GetMaxFPS"] = appTable["GetMaxFPS"];

    auto windowTable = lua.create_named_table("Window");
    windowTable["SetTitle"] = [](const std::string& title) {
        if (Engine::Get()) Engine::Get()->GetWindow().SetTitle(title);
    };
    windowTable["GetTitle"] = []() -> std::string {
        return Engine::Get() ? Engine::Get()->GetWindow().GetTitle() : "";
    };
    windowTable["SetSize"] = [](int width, int height) {
        if (Engine::Get()) Engine::Get()->GetWindow().SetSize(width, height);
    };
    windowTable["GetWidth"] = []() -> int {
        return Engine::Get() ? Engine::Get()->GetWindow().GetSize().width : 0;
    };
    windowTable["GetHeight"] = []() -> int {
        return Engine::Get() ? Engine::Get()->GetWindow().GetSize().height : 0;
    };
    windowTable["GetSize"] = [](sol::this_state s) -> sol::object {
        if (!Engine::Get()) return sol::nil;
        auto sz = Engine::Get()->GetWindow().GetSize();
        return sol::make_object(s, glm::vec2(static_cast<float>(sz.width), static_cast<float>(sz.height)));
    };
    windowTable["Center"] = []() {
        if (Engine::Get()) Engine::Get()->GetWindow().CenterOnMonitor();
    };
    windowTable["SetVSync"] = [](bool vsync) {
        if (Engine::Get()) Engine::Get()->GetWindow().SetVSync(vsync);
    };
    windowTable["IsVSync"] = []() -> bool {
        return Engine::Get() ? Engine::Get()->GetWindow().IsVSync() : false;
    };
    windowTable["SetWindowMode"] = [](int mode) {
        if (Engine::Get()) Engine::Get()->GetWindow().SetWindowMode(static_cast<WindowMode>(mode));
    };
    windowTable["SetFullscreen"] = [](bool fullscreen) {
        if (Engine::Get()) Engine::Get()->GetWindow().SetWindowMode(fullscreen ? WindowMode::Fullscreen : WindowMode::Windowed);
    };
    windowTable["SetMaximized"] = [](bool max) {
        if (Engine::Get()) {
            if (max) Engine::Get()->GetWindow().Maximize();
            else Engine::Get()->GetWindow().Restore();
        }
    };
    windowTable["IsMaximized"] = []() -> bool {
        return Engine::Get() ? Engine::Get()->GetWindow().IsMaximized() : false;
    };
    windowTable["SetBorderless"] = [](bool borderless) {
        if (Engine::Get()) Engine::Get()->GetWindow().SetBorderless(borderless);
    };
    windowTable["SetResizable"] = [](bool resizable) {
        if (Engine::Get()) Engine::Get()->GetWindow().SetResizable(resizable);
    };
    windowTable["SetIcon"] = [](const std::string& path) -> bool {
        return Engine::Get() ? Engine::Get()->GetWindow().SetIcon(path) : false;
    };
    windowTable["ResetIcon"] = []() {
        if (Engine::Get()) Engine::Get()->GetWindow().ResetIcon();
    };
    windowTable["SetCursorIcon"] = [](const std::string& path, sol::optional<int> hotX, sol::optional<int> hotY) -> bool {
        return Engine::Get() ? Engine::Get()->GetWindow().SetCursorIcon(path, hotX.value_or(0), hotY.value_or(0)) : false;
    };
    windowTable["SetCustomCursor"] = windowTable["SetCursorIcon"];
    windowTable["ResetCursor"] = []() {
        if (Engine::Get()) Engine::Get()->GetWindow().ResetCursor();
    };
    windowTable["SetStandardCursor"] = [](int shape) {
        if (Engine::Get()) Engine::Get()->GetWindow().SetStandardCursor(static_cast<CursorShape>(shape));
    };

    auto cursorShapeEnum = lua.create_named_table("CursorShape");
    cursorShapeEnum["Arrow"] = static_cast<int>(CursorShape::Arrow);
    cursorShapeEnum["IBeam"] = static_cast<int>(CursorShape::IBeam);
    cursorShapeEnum["Crosshair"] = static_cast<int>(CursorShape::Crosshair);
    cursorShapeEnum["Hand"] = static_cast<int>(CursorShape::Hand);
    cursorShapeEnum["HResize"] = static_cast<int>(CursorShape::HResize);
    cursorShapeEnum["VResize"] = static_cast<int>(CursorShape::VResize);

    auto windowModeEnum = lua.create_named_table("WindowMode");
    windowModeEnum["Windowed"] = static_cast<int>(WindowMode::Windowed);
    windowModeEnum["Minimized"] = static_cast<int>(WindowMode::Minimized);
    windowModeEnum["Maximized"] = static_cast<int>(WindowMode::Maximized);
    windowModeEnum["Fullscreen"] = static_cast<int>(WindowMode::Fullscreen);

    auto storageTable = lua.create_named_table("Storage");
    storageTable["GetSaveDirectory"] = []() -> std::string {
        std::filesystem::path saveDir = ProjectManager::IsProjectLoaded()
            ? (ProjectManager::GetProjectPath() / "Saves")
            : (Path::GetExecutableDirectory() / "Saves");
        std::error_code ec;
        std::filesystem::create_directories(saveDir, ec);
        return saveDir.string();
    };

    storageTable["SaveText"] = [](const std::string& filename, const std::string& content) -> bool {
        std::filesystem::path saveDir = ProjectManager::IsProjectLoaded()
            ? (ProjectManager::GetProjectPath() / "Saves")
            : (Path::GetExecutableDirectory() / "Saves");
        std::error_code ec;
        std::filesystem::create_directories(saveDir, ec);
        std::filesystem::path filePath = saveDir / filename;
        std::ofstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR("Storage: Failed to open file for writing: {}", filePath.string());
            return false;
        }
        file << content;
        return true;
    };

    storageTable["LoadText"] = [](sol::this_state s, const std::string& filename) -> sol::object {
        std::filesystem::path saveDir = ProjectManager::IsProjectLoaded()
            ? (ProjectManager::GetProjectPath() / "Saves")
            : (Path::GetExecutableDirectory() / "Saves");
        std::filesystem::path filePath = saveDir / filename;
        if (!std::filesystem::exists(filePath)) {
            return sol::nil;
        }
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR("Storage: Failed to open file for reading: {}", filePath.string());
            return sol::nil;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return sol::make_object(s, buffer.str());
    };

    storageTable["FileExists"] = [](const std::string& filename) -> bool {
        std::filesystem::path saveDir = ProjectManager::IsProjectLoaded()
            ? (ProjectManager::GetProjectPath() / "Saves")
            : (Path::GetExecutableDirectory() / "Saves");
        return std::filesystem::exists(saveDir / filename);
    };

    storageTable["DeleteFile"] = [](const std::string& filename) -> bool {
        std::filesystem::path saveDir = ProjectManager::IsProjectLoaded()
            ? (ProjectManager::GetProjectPath() / "Saves")
            : (Path::GetExecutableDirectory() / "Saves");
        std::error_code ec;
        return std::filesystem::remove(saveDir / filename, ec);
    };

    lua["SaveSystem"] = storageTable;

    auto projSettingsTable = lua.create_named_table("ProjectSettings");

    // General / Project Info
    projSettingsTable["GetName"] = []() -> std::string {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().name : "";
    };
    projSettingsTable["SetName"] = [](const std::string& name) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) {
            activeProj->GetSettings().name = name;
            activeProj->name = name;
        }
    };
    projSettingsTable["GetStartScene"] = []() -> std::string {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().startScene : "";
    };
    projSettingsTable["SetStartScene"] = [](const std::string& scene) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().startScene = scene;
    };
    projSettingsTable["GetProjectPath"] = []() -> std::string {
        return ProjectManager::IsProjectLoaded() ? ProjectManager::GetProjectPath().string() : "";
    };
    projSettingsTable["GetAssetDirectory"] = []() -> std::string {
        return ProjectManager::IsProjectLoaded() ? ProjectManager::GetAssetDirectory().string() : "";
    };
    projSettingsTable["Save"] = []() -> bool {
        if (!ProjectManager::IsProjectLoaded()) return false;
        ProjectManager::SaveActiveProject();
        return true;
    };

    // Viewport & Window
    projSettingsTable["GetViewportWidth"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().viewportWidth : 1920;
    };
    projSettingsTable["GetViewportHeight"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().viewportHeight : 1080;
    };
    projSettingsTable["GetViewportSize"] = []() -> glm::vec2 {
        auto activeProj = ProjectManager::GetActiveProject();
        if (!activeProj) return glm::vec2(1920.0f, 1080.0f);
        return glm::vec2(static_cast<float>(activeProj->GetSettings().viewportWidth), static_cast<float>(activeProj->GetSettings().viewportHeight));
    };
    projSettingsTable["SetViewportSize"] = sol::overload(
        [](uint32_t width, uint32_t height) {
            auto activeProj = ProjectManager::GetActiveProject();
            if (activeProj) {
                activeProj->GetSettings().viewportWidth = std::max(1u, width);
                activeProj->GetSettings().viewportHeight = std::max(1u, height);
            }
        },
        [](const glm::vec2& size) {
            auto activeProj = ProjectManager::GetActiveProject();
            if (activeProj) {
                activeProj->GetSettings().viewportWidth = static_cast<uint32_t>(std::max(1.0f, size.x));
                activeProj->GetSettings().viewportHeight = static_cast<uint32_t>(std::max(1.0f, size.y));
            }
        }
    );
    projSettingsTable["GetWindowWidth"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().windowWidth : 1600;
    };
    projSettingsTable["GetWindowHeight"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().windowHeight : 900;
    };
    projSettingsTable["GetWindowSize"] = []() -> glm::vec2 {
        auto activeProj = ProjectManager::GetActiveProject();
        if (!activeProj) return glm::vec2(1600.0f, 900.0f);
        return glm::vec2(static_cast<float>(activeProj->GetSettings().windowWidth), static_cast<float>(activeProj->GetSettings().windowHeight));
    };
    projSettingsTable["SetWindowSize"] = sol::overload(
        [](uint32_t width, uint32_t height) {
            auto activeProj = ProjectManager::GetActiveProject();
            if (activeProj) {
                activeProj->GetSettings().windowWidth = std::max(1u, width);
                activeProj->GetSettings().windowHeight = std::max(1u, height);
                if (Engine::Get()) Engine::Get()->GetWindow().SetSize(static_cast<int>(width), static_cast<int>(height));
            }
        },
        [](const glm::vec2& size) {
            auto activeProj = ProjectManager::GetActiveProject();
            if (activeProj) {
                uint32_t width = static_cast<uint32_t>(std::max(1.0f, size.x));
                uint32_t height = static_cast<uint32_t>(std::max(1.0f, size.y));
                activeProj->GetSettings().windowWidth = width;
                activeProj->GetSettings().windowHeight = height;
                if (Engine::Get()) Engine::Get()->GetWindow().SetSize(static_cast<int>(width), static_cast<int>(height));
            }
        }
    );
    projSettingsTable["GetWindowMode"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? static_cast<int>(activeProj->GetSettings().windowMode) : 0;
    };
    projSettingsTable["SetWindowMode"] = [](int mode) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) {
            activeProj->GetSettings().windowMode = static_cast<WindowMode>(mode);
            if (Engine::Get()) Engine::Get()->GetWindow().SetWindowMode(static_cast<WindowMode>(mode));
        }
    };
    projSettingsTable["IsResizable"] = []() -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().resizable : true;
    };
    projSettingsTable["SetResizable"] = [](bool resizable) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) {
            activeProj->GetSettings().resizable = resizable;
            if (Engine::Get()) Engine::Get()->GetWindow().SetResizable(resizable);
        }
    };
    projSettingsTable["IsBorderless"] = []() -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().borderless : false;
    };
    projSettingsTable["SetBorderless"] = [](bool borderless) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) {
            activeProj->GetSettings().borderless = borderless;
            if (Engine::Get()) Engine::Get()->GetWindow().SetBorderless(borderless);
        }
    };
    projSettingsTable["GetVSync"] = []() -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().vsync : true;
    };
    projSettingsTable["SetVSync"] = [](bool vsync) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) {
            activeProj->GetSettings().vsync = vsync;
            if (Engine::Get()) Engine::Get()->GetWindow().SetVSync(vsync);
        }
    };
    projSettingsTable["GetMaxFPS"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().maxFPS : 0;
    };
    projSettingsTable["SetMaxFPS"] = [](int maxFPS) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) {
            activeProj->GetSettings().maxFPS = std::max(0, maxFPS);
            Time::SetMaxFPS(std::max(0, maxFPS));
        }
    };
    projSettingsTable["GetStretchMode"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? static_cast<int>(activeProj->GetSettings().stretchMode) : 2;
    };
    projSettingsTable["SetStretchMode"] = [](int mode) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().stretchMode = static_cast<StretchMode>(mode);
    };
    projSettingsTable["GetStretchAspect"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? static_cast<int>(activeProj->GetSettings().stretchAspect) : 1;
    };
    projSettingsTable["SetStretchAspect"] = [](int aspect) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().stretchAspect = static_cast<StretchAspect>(aspect);
    };
    projSettingsTable["GetScalingFilter"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? static_cast<int>(activeProj->GetSettings().scalingFilter) : 1;
    };
    projSettingsTable["SetScalingFilter"] = [](int filter) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().scalingFilter = static_cast<ScalingFilter>(filter);
    };

    // Rendering Quality
    projSettingsTable["GetShadowResolution"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().shadowResolution : 2048;
    };
    projSettingsTable["SetShadowResolution"] = [](uint32_t res) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().shadowResolution = res;
    };
    projSettingsTable["GetPointShadowResolution"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().pointShadowResolution : 512;
    };
    projSettingsTable["SetPointShadowResolution"] = [](uint32_t res) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().pointShadowResolution = res;
    };
    projSettingsTable["GetSpotShadowResolution"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().spotShadowResolution : 1024;
    };
    projSettingsTable["SetSpotShadowResolution"] = [](uint32_t res) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().spotShadowResolution = res;
    };
    projSettingsTable["IsFXAAEnabled"] = []() -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().fxaaEnabled : true;
    };
    projSettingsTable["SetFXAAEnabled"] = [](bool enabled) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().fxaaEnabled = enabled;
    };
    projSettingsTable["IsSSAOHalfResolution"] = []() -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().ssaoHalfResolution : true;
    };
    projSettingsTable["SetSSAOHalfResolution"] = [](bool halfRes) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().ssaoHalfResolution = halfRes;
    };
    projSettingsTable["GetSSAOSliceCount"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().ssaoSliceCount : 3;
    };
    projSettingsTable["SetSSAOSliceCount"] = [](int slices) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().ssaoSliceCount = std::clamp(slices, 1, 6);
    };
    projSettingsTable["GetSSAOStepCount"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().ssaoStepCount : 3;
    };
    projSettingsTable["SetSSAOStepCount"] = [](int steps) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().ssaoStepCount = std::clamp(steps, 1, 8);
    };
    projSettingsTable["IsContactShadowsHalfResolution"] = []() -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().contactShadowsHalfResolution : true;
    };
    projSettingsTable["SetContactShadowsHalfResolution"] = [](bool halfRes) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().contactShadowsHalfResolution = halfRes;
    };
    projSettingsTable["GetContactShadowsSteps"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().contactShadowsSteps : 16;
    };
    projSettingsTable["SetContactShadowsSteps"] = [](int steps) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().contactShadowsSteps = std::clamp(steps, 4, 64);
    };
    projSettingsTable["IsSSRHalfResolution"] = []() -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().ssrHalfResolution : true;
    };
    projSettingsTable["SetSSRHalfResolution"] = [](bool halfRes) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().ssrHalfResolution = halfRes;
    };
    projSettingsTable["GetSSRMaxSteps"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().ssrMaxSteps : 64;
    };
    projSettingsTable["SetSSRMaxSteps"] = [](int steps) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().ssrMaxSteps = std::clamp(steps, 8, 128);
    };
    projSettingsTable["IsSSGIHalfResolution"] = []() -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().ssgiHalfResolution : true;
    };
    projSettingsTable["SetSSGIHalfResolution"] = [](bool halfRes) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().ssgiHalfResolution = halfRes;
    };
    projSettingsTable["GetSSGISteps"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().ssgiSteps : 16;
    };
    projSettingsTable["SetSSGISteps"] = [](int steps) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().ssgiSteps = std::clamp(steps, 4, 64);
    };
    projSettingsTable["IsVolumetricFogHalfResolution"] = []() -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().volumetricFogHalfResolution : true;
    };
    projSettingsTable["SetVolumetricFogHalfResolution"] = [](bool halfRes) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().volumetricFogHalfResolution = halfRes;
    };
    projSettingsTable["GetVolumetricFogSteps"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().volumetricFogSteps : 24;
    };
    projSettingsTable["SetVolumetricFogSteps"] = [](int steps) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().volumetricFogSteps = std::clamp(steps, 8, 128);
    };
    projSettingsTable["GetMotionBlurSamples"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().motionBlurSamples : 16;
    };
    projSettingsTable["SetMotionBlurSamples"] = [](int samples) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().motionBlurSamples = std::clamp(samples, 4, 32);
    };
    projSettingsTable["GetDefaultTextureFilter"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? static_cast<int>(activeProj->GetSettings().defaultTextureFilter) : 4;
    };
    projSettingsTable["SetDefaultTextureFilter"] = [](int filter) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().defaultTextureFilter = static_cast<TextureFilter>(filter);
    };
    projSettingsTable["GetDefaultTextureWrap"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? static_cast<int>(activeProj->GetSettings().defaultTextureWrap) : 1;
    };
    projSettingsTable["SetDefaultTextureWrap"] = [](int wrap) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().defaultTextureWrap = static_cast<TextureWrap>(wrap);
    };
    projSettingsTable["GetDefaultAnisotropy"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? static_cast<int>(activeProj->GetSettings().defaultAnisotropy) : 4;
    };
    projSettingsTable["SetDefaultAnisotropy"] = [](int aniso) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().defaultAnisotropy = static_cast<TextureAnisotropy>(aniso);
    };

    // Physics Settings
    projSettingsTable["GetGravity"] = []() -> glm::vec3 {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().physicsGravity : glm::vec3(0.0f, -9.81f, 0.0f);
    };
    projSettingsTable["SetGravity"] = sol::overload(
        [](const glm::vec3& gravity) {
            auto activeProj = ProjectManager::GetActiveProject();
            if (activeProj) activeProj->GetSettings().physicsGravity = gravity;
        },
        [](float x, float y, float z) {
            auto activeProj = ProjectManager::GetActiveProject();
            if (activeProj) activeProj->GetSettings().physicsGravity = glm::vec3(x, y, z);
        }
    );
    projSettingsTable["GetPhysicsTickRate"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().physicsTicksPerSecond : 60;
    };
    projSettingsTable["SetPhysicsTickRate"] = [](int hz) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) {
            activeProj->GetSettings().physicsTicksPerSecond = std::clamp(hz, 15, 240);
            Time::SetFixedDeltaTime(1.0f / static_cast<float>(activeProj->GetSettings().physicsTicksPerSecond));
        }
    };
    projSettingsTable["GetFixedTimestep"] = []() -> float {
        auto activeProj = ProjectManager::GetActiveProject();
        int hz = activeProj ? activeProj->GetSettings().physicsTicksPerSecond : 60;
        return 1.0f / static_cast<float>(std::max(1, hz));
    };
    projSettingsTable["GetPhysicsSubSteps"] = []() -> int {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().physicsSubSteps : 1;
    };
    projSettingsTable["SetPhysicsSubSteps"] = [](int steps) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().physicsSubSteps = std::clamp(steps, 1, 16);
    };
    projSettingsTable["GetPhysicsMaxBodies"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().physicsMaxBodies : 1024;
    };
    projSettingsTable["SetPhysicsMaxBodies"] = [](uint32_t maxBodies) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().physicsMaxBodies = maxBodies;
    };
    projSettingsTable["GetPhysicsMaxBodyPairs"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().physicsMaxBodyPairs : 1024;
    };
    projSettingsTable["SetPhysicsMaxBodyPairs"] = [](uint32_t maxPairs) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().physicsMaxBodyPairs = maxPairs;
    };
    projSettingsTable["GetPhysicsMaxContactConstraints"] = []() -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        return activeProj ? activeProj->GetSettings().physicsMaxContactConstraints : 1024;
    };
    projSettingsTable["SetPhysicsMaxContactConstraints"] = [](uint32_t maxContacts) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().physicsMaxContactConstraints = maxContacts;
    };
    projSettingsTable["GetPhysicsLayerName"] = [](uint32_t layer) -> std::string {
        auto activeProj = ProjectManager::GetActiveProject();
        if (!activeProj) return "";
        if (layer == 0) return activeProj->GetSettings().GetPhysicsLayerNameByBit(0);
        return activeProj->GetSettings().GetPhysicsLayerName(layer);
    };
    projSettingsTable["SetPhysicsLayerName"] = [](uint32_t layer, const std::string& name) {
        auto activeProj = ProjectManager::GetActiveProject();
        if (activeProj) activeProj->GetSettings().SetPhysicsLayerName(layer, name);
    };
    projSettingsTable["GetPhysicsLayerMask"] = [](sol::variadic_args va) -> uint32_t {
        auto activeProj = ProjectManager::GetActiveProject();
        if (!activeProj || va.size() == 0) return 0;
        uint32_t mask = 0;
        for (auto item : va) {
            if (item.is<std::string>()) {
                mask |= activeProj->GetSettings().GetPhysicsLayerMask(item.as<std::string>());
            } else if (item.is<uint32_t>()) {
                uint32_t l = item.as<uint32_t>();
                if (l >= 1 && l <= 32) mask |= (1u << (l - 1));
                else if (l == 0) mask |= 1u;
            }
        }
        return mask;
    };

    auto normalizeKey = [](std::string key) -> std::string {
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return key;
    };

    projSettingsTable["Get"] = [normalizeKey](sol::this_state s, const std::string& rawKey) -> sol::object {
        auto activeProj = ProjectManager::GetActiveProject();
        if (!activeProj) return sol::nil;
        auto& settings = activeProj->GetSettings();
        std::string key = normalizeKey(rawKey);

        if (key == "application/name" || key == "name") return sol::make_object(s, settings.name);
        if (key == "application/start_scene" || key == "start_scene") return sol::make_object(s, settings.startScene);

        if (key == "display/window/width") return sol::make_object(s, settings.windowWidth);
        if (key == "display/window/height") return sol::make_object(s, settings.windowHeight);
        if (key == "display/window/size") return sol::make_object(s, glm::vec2(static_cast<float>(settings.windowWidth), static_cast<float>(settings.windowHeight)));
        if (key == "display/window/mode") return sol::make_object(s, static_cast<int>(settings.windowMode));
        if (key == "display/window/resizable") return sol::make_object(s, settings.resizable);
        if (key == "display/window/borderless") return sol::make_object(s, settings.borderless);
        if (key == "display/window/vsync") return sol::make_object(s, settings.vsync);
        if (key == "display/window/max_fps") return sol::make_object(s, settings.maxFPS);
        if (key == "display/window/stretch_mode") return sol::make_object(s, static_cast<int>(settings.stretchMode));
        if (key == "display/window/stretch_aspect") return sol::make_object(s, static_cast<int>(settings.stretchAspect));
        if (key == "display/window/scaling_filter") return sol::make_object(s, static_cast<int>(settings.scalingFilter));

        if (key == "rendering/shadows/directional_resolution" || key == "rendering/shadow_resolution") return sol::make_object(s, settings.shadowResolution);
        if (key == "rendering/shadows/point_resolution") return sol::make_object(s, settings.pointShadowResolution);
        if (key == "rendering/shadows/spot_resolution") return sol::make_object(s, settings.spotShadowResolution);
        if (key == "rendering/anti_aliasing/fxaa" || key == "rendering/fxaa") return sol::make_object(s, settings.fxaaEnabled);
        if (key == "rendering/ssao/half_resolution") return sol::make_object(s, settings.ssaoHalfResolution);
        if (key == "rendering/ssao/slice_count") return sol::make_object(s, settings.ssaoSliceCount);
        if (key == "rendering/ssao/step_count") return sol::make_object(s, settings.ssaoStepCount);
        if (key == "rendering/contact_shadows/half_resolution") return sol::make_object(s, settings.contactShadowsHalfResolution);
        if (key == "rendering/contact_shadows/steps") return sol::make_object(s, settings.contactShadowsSteps);
        if (key == "rendering/ssr/half_resolution") return sol::make_object(s, settings.ssrHalfResolution);
        if (key == "rendering/ssr/max_steps") return sol::make_object(s, settings.ssrMaxSteps);
        if (key == "rendering/ssgi/half_resolution") return sol::make_object(s, settings.ssgiHalfResolution);
        if (key == "rendering/ssgi/steps") return sol::make_object(s, settings.ssgiSteps);
        if (key == "rendering/volumetric_fog/half_resolution") return sol::make_object(s, settings.volumetricFogHalfResolution);
        if (key == "rendering/volumetric_fog/steps") return sol::make_object(s, settings.volumetricFogSteps);
        if (key == "rendering/motion_blur/samples") return sol::make_object(s, settings.motionBlurSamples);
        if (key == "rendering/textures/filter") return sol::make_object(s, static_cast<int>(settings.defaultTextureFilter));
        if (key == "rendering/textures/wrap") return sol::make_object(s, static_cast<int>(settings.defaultTextureWrap));
        if (key == "rendering/textures/anisotropy") return sol::make_object(s, static_cast<int>(settings.defaultAnisotropy));

        if (key == "physics/gravity") return sol::make_object(s, settings.physicsGravity);
        if (key == "physics/tick_rate" || key == "physics/ticks_per_second") return sol::make_object(s, settings.physicsTicksPerSecond);
        if (key == "physics/sub_steps") return sol::make_object(s, settings.physicsSubSteps);
        if (key == "physics/max_bodies") return sol::make_object(s, settings.physicsMaxBodies);
        if (key == "physics/max_body_pairs") return sol::make_object(s, settings.physicsMaxBodyPairs);
        if (key == "physics/max_contact_constraints") return sol::make_object(s, settings.physicsMaxContactConstraints);

        return sol::nil;
    };

    projSettingsTable["Set"] = [normalizeKey](const std::string& rawKey, sol::object val) -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        if (!activeProj) return false;
        auto& settings = activeProj->GetSettings();
        std::string key = normalizeKey(rawKey);

        if (key == "application/name" || key == "name") {
            if (val.is<std::string>()) { settings.name = val.as<std::string>(); activeProj->name = settings.name; return true; }
        }
        else if (key == "application/start_scene" || key == "start_scene") {
            if (val.is<std::string>()) { settings.startScene = val.as<std::string>(); return true; }
        }
        else if (key == "display/window/width") {
            if (val.is<uint32_t>()) { settings.windowWidth = std::max(1u, val.as<uint32_t>()); return true; }
        }
        else if (key == "display/window/height") {
            if (val.is<uint32_t>()) { settings.windowHeight = std::max(1u, val.as<uint32_t>()); return true; }
        }
        else if (key == "display/window/size") {
            if (val.is<glm::vec2>()) {
                auto sz = val.as<glm::vec2>();
                settings.windowWidth = static_cast<uint32_t>(std::max(1.0f, sz.x));
                settings.windowHeight = static_cast<uint32_t>(std::max(1.0f, sz.y));
                if (Engine::Get()) Engine::Get()->GetWindow().SetSize(static_cast<int>(settings.windowWidth), static_cast<int>(settings.windowHeight));
                return true;
            }
        }
        else if (key == "display/window/mode") {
            if (val.is<int>()) {
                settings.windowMode = static_cast<WindowMode>(val.as<int>());
                if (Engine::Get()) Engine::Get()->GetWindow().SetWindowMode(settings.windowMode);
                return true;
            }
        }
        else if (key == "display/window/resizable") {
            if (val.is<bool>()) {
                settings.resizable = val.as<bool>();
                if (Engine::Get()) Engine::Get()->GetWindow().SetResizable(settings.resizable);
                return true;
            }
        }
        else if (key == "display/window/borderless") {
            if (val.is<bool>()) {
                settings.borderless = val.as<bool>();
                if (Engine::Get()) Engine::Get()->GetWindow().SetBorderless(settings.borderless);
                return true;
            }
        }
        else if (key == "display/window/vsync") {
            if (val.is<bool>()) {
                settings.vsync = val.as<bool>();
                if (Engine::Get()) Engine::Get()->GetWindow().SetVSync(settings.vsync);
                return true;
            }
        }
        else if (key == "display/window/max_fps") {
            if (val.is<int>()) {
                settings.maxFPS = std::max(0, val.as<int>());
                Time::SetMaxFPS(settings.maxFPS);
                return true;
            }
        }
        else if (key == "display/window/stretch_mode") {
            if (val.is<int>()) { settings.stretchMode = static_cast<StretchMode>(val.as<int>()); return true; }
        }
        else if (key == "display/window/stretch_aspect") {
            if (val.is<int>()) { settings.stretchAspect = static_cast<StretchAspect>(val.as<int>()); return true; }
        }
        else if (key == "display/window/scaling_filter") {
            if (val.is<int>()) { settings.scalingFilter = static_cast<ScalingFilter>(val.as<int>()); return true; }
        }
        else if (key == "rendering/shadows/directional_resolution" || key == "rendering/shadow_resolution") {
            if (val.is<uint32_t>()) { settings.shadowResolution = val.as<uint32_t>(); return true; }
        }
        else if (key == "rendering/shadows/point_resolution") {
            if (val.is<uint32_t>()) { settings.pointShadowResolution = val.as<uint32_t>(); return true; }
        }
        else if (key == "rendering/shadows/spot_resolution") {
            if (val.is<uint32_t>()) { settings.spotShadowResolution = val.as<uint32_t>(); return true; }
        }
        else if (key == "rendering/anti_aliasing/fxaa" || key == "rendering/fxaa") {
            if (val.is<bool>()) { settings.fxaaEnabled = val.as<bool>(); return true; }
        }
        else if (key == "rendering/ssao/half_resolution") {
            if (val.is<bool>()) { settings.ssaoHalfResolution = val.as<bool>(); return true; }
        }
        else if (key == "rendering/ssao/slice_count") {
            if (val.is<int>()) { settings.ssaoSliceCount = std::clamp(val.as<int>(), 1, 6); return true; }
        }
        else if (key == "rendering/ssao/step_count") {
            if (val.is<int>()) { settings.ssaoStepCount = std::clamp(val.as<int>(), 1, 8); return true; }
        }
        else if (key == "rendering/contact_shadows/half_resolution") {
            if (val.is<bool>()) { settings.contactShadowsHalfResolution = val.as<bool>(); return true; }
        }
        else if (key == "rendering/contact_shadows/steps") {
            if (val.is<int>()) { settings.contactShadowsSteps = std::clamp(val.as<int>(), 4, 64); return true; }
        }
        else if (key == "rendering/ssr/half_resolution") {
            if (val.is<bool>()) { settings.ssrHalfResolution = val.as<bool>(); return true; }
        }
        else if (key == "rendering/ssr/max_steps") {
            if (val.is<int>()) { settings.ssrMaxSteps = std::clamp(val.as<int>(), 8, 128); return true; }
        }
        else if (key == "rendering/ssgi/half_resolution") {
            if (val.is<bool>()) { settings.ssgiHalfResolution = val.as<bool>(); return true; }
        }
        else if (key == "rendering/ssgi/steps") {
            if (val.is<int>()) { settings.ssgiSteps = std::clamp(val.as<int>(), 4, 64); return true; }
        }
        else if (key == "rendering/volumetric_fog/half_resolution") {
            if (val.is<bool>()) { settings.volumetricFogHalfResolution = val.as<bool>(); return true; }
        }
        else if (key == "rendering/volumetric_fog/steps") {
            if (val.is<int>()) { settings.volumetricFogSteps = std::clamp(val.as<int>(), 8, 128); return true; }
        }
        else if (key == "rendering/motion_blur/samples") {
            if (val.is<int>()) { settings.motionBlurSamples = std::clamp(val.as<int>(), 4, 32); return true; }
        }
        else if (key == "rendering/textures/filter") {
            if (val.is<int>()) { settings.defaultTextureFilter = static_cast<TextureFilter>(val.as<int>()); return true; }
        }
        else if (key == "rendering/textures/wrap") {
            if (val.is<int>()) { settings.defaultTextureWrap = static_cast<TextureWrap>(val.as<int>()); return true; }
        }
        else if (key == "rendering/textures/anisotropy") {
            if (val.is<int>()) { settings.defaultAnisotropy = static_cast<TextureAnisotropy>(val.as<int>()); return true; }
        }
        else if (key == "physics/gravity") {
            if (val.is<glm::vec3>()) { settings.physicsGravity = val.as<glm::vec3>(); return true; }
        }
        else if (key == "physics/tick_rate" || key == "physics/ticks_per_second") {
            if (val.is<int>()) {
                settings.physicsTicksPerSecond = std::clamp(val.as<int>(), 15, 240);
                Time::SetFixedDeltaTime(1.0f / static_cast<float>(settings.physicsTicksPerSecond));
                return true;
            }
        }
        else if (key == "physics/sub_steps") {
            if (val.is<int>()) { settings.physicsSubSteps = std::clamp(val.as<int>(), 1, 16); return true; }
        }
        else if (key == "physics/max_bodies") {
            if (val.is<uint32_t>()) { settings.physicsMaxBodies = val.as<uint32_t>(); return true; }
        }
        else if (key == "physics/max_body_pairs") {
            if (val.is<uint32_t>()) { settings.physicsMaxBodyPairs = val.as<uint32_t>(); return true; }
        }
        else if (key == "physics/max_contact_constraints") {
            if (val.is<uint32_t>()) { settings.physicsMaxContactConstraints = val.as<uint32_t>(); return true; }
        }

        return false;
    };

    projSettingsTable["Has"] = [normalizeKey](const std::string& rawKey) -> bool {
        auto activeProj = ProjectManager::GetActiveProject();
        if (!activeProj) return false;
        std::string key = normalizeKey(rawKey);
        static const std::unordered_set<std::string> s_ValidKeys = {
            "application/name", "name", "application/start_scene", "start_scene",
            "display/window/width", "display/window/height", "display/window/size",
            "display/window/mode", "display/window/resizable", "display/window/borderless",
            "display/window/vsync", "display/window/max_fps", "display/window/stretch_mode",
            "display/window/stretch_aspect", "display/window/scaling_filter",
            "rendering/shadows/directional_resolution", "rendering/shadow_resolution",
            "rendering/shadows/point_resolution", "rendering/shadows/spot_resolution",
            "rendering/anti_aliasing/fxaa", "rendering/fxaa",
            "rendering/ssao/half_resolution", "rendering/ssao/slice_count", "rendering/ssao/step_count",
            "rendering/contact_shadows/half_resolution", "rendering/contact_shadows/steps",
            "rendering/ssr/half_resolution", "rendering/ssr/max_steps",
            "rendering/ssgi/half_resolution", "rendering/ssgi/steps",
            "rendering/volumetric_fog/half_resolution", "rendering/volumetric_fog/steps",
            "rendering/motion_blur/samples", "rendering/textures/filter", "rendering/textures/wrap",
            "rendering/textures/anisotropy", "physics/gravity", "physics/tick_rate",
            "physics/ticks_per_second", "physics/sub_steps", "physics/max_bodies",
            "physics/max_body_pairs", "physics/max_contact_constraints"
        };
        return s_ValidKeys.find(key) != s_ValidKeys.end();
    };

    lua["ProjectSettings"] = projSettingsTable;
    lua["Project"] = projSettingsTable;

    // Enums
    auto stretchModeEnum = lua.create_named_table("StretchMode");
    stretchModeEnum["Disabled"] = static_cast<int>(StretchMode::Disabled);
    stretchModeEnum["CanvasItems"] = static_cast<int>(StretchMode::CanvasItems);
    stretchModeEnum["Viewport"] = static_cast<int>(StretchMode::Viewport);

    auto stretchAspectEnum = lua.create_named_table("StretchAspect");
    stretchAspectEnum["Ignore"] = static_cast<int>(StretchAspect::Ignore);
    stretchAspectEnum["Keep"] = static_cast<int>(StretchAspect::Keep);
    stretchAspectEnum["KeepWidth"] = static_cast<int>(StretchAspect::KeepWidth);
    stretchAspectEnum["KeepHeight"] = static_cast<int>(StretchAspect::KeepHeight);
    stretchAspectEnum["Expand"] = static_cast<int>(StretchAspect::Expand);

    auto scalingFilterEnum = lua.create_named_table("ScalingFilter");
    scalingFilterEnum["Linear"] = static_cast<int>(ScalingFilter::Linear);
    scalingFilterEnum["Nearest"] = static_cast<int>(ScalingFilter::Nearest);

    auto textureFilterEnum = lua.create_named_table("TextureFilter");
    textureFilterEnum["Default"] = static_cast<int>(TextureFilter::Default);
    textureFilterEnum["Nearest"] = static_cast<int>(TextureFilter::Nearest);
    textureFilterEnum["Linear"] = static_cast<int>(TextureFilter::Linear);
    textureFilterEnum["NearestMipmap"] = static_cast<int>(TextureFilter::NearestMipmap);
    textureFilterEnum["LinearMipmap"] = static_cast<int>(TextureFilter::LinearMipmap);

    auto textureWrapEnum = lua.create_named_table("TextureWrap");
    textureWrapEnum["Default"] = static_cast<int>(TextureWrap::Default);
    textureWrapEnum["Repeat"] = static_cast<int>(TextureWrap::Repeat);
    textureWrapEnum["MirroredRepeat"] = static_cast<int>(TextureWrap::MirroredRepeat);
    textureWrapEnum["MirrorRepeat"] = static_cast<int>(TextureWrap::MirroredRepeat);
    textureWrapEnum["ClampToEdge"] = static_cast<int>(TextureWrap::ClampToEdge);
    textureWrapEnum["ClampToBorder"] = static_cast<int>(TextureWrap::ClampToBorder);
    textureWrapEnum["MirroredClampToEdge"] = static_cast<int>(TextureWrap::MirroredClampToEdge);
    textureWrapEnum["MirrorClampToEdge"] = static_cast<int>(TextureWrap::MirroredClampToEdge);

    auto textureAnisoEnum = lua.create_named_table("TextureAnisotropy");
    textureAnisoEnum["Default"] = static_cast<int>(TextureAnisotropy::Default);
    textureAnisoEnum["Off"] = static_cast<int>(TextureAnisotropy::Off);
    textureAnisoEnum["Disabled"] = static_cast<int>(TextureAnisotropy::Off);
    textureAnisoEnum["Aniso2x"] = static_cast<int>(TextureAnisotropy::Aniso2x);
    textureAnisoEnum["Aniso4x"] = static_cast<int>(TextureAnisotropy::Aniso4x);
    textureAnisoEnum["Aniso8x"] = static_cast<int>(TextureAnisotropy::Aniso8x);
    textureAnisoEnum["Aniso16x"] = static_cast<int>(TextureAnisotropy::Aniso16x);
}
