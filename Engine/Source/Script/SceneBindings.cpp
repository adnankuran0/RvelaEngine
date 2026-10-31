#include "rvelapch.h"
#include "SceneBindings.h"
#include "sol/sol.hpp"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetUUID.h"
#include <glm/gtc/quaternion.hpp>
#include "Scene/Components/AutoloadComponent.h"
#include "Scene/Components/ScriptComponent.h"

using namespace rv;

namespace {

struct TargetArg
{
    enum class Type { Invalid, Id, Path, Handle };
    Type type = Type::Invalid;
    uint32_t id = 0;
    std::string path;
    AssetUUID handle = AssetUUID::Invalid();
};

inline std::vector<sol::object> GetCleanArgs(sol::variadic_args va)
{
    std::vector<sol::object> list;
    for (auto obj : va) list.push_back(obj);
    if (!list.empty() && list[0].is<sol::table>())
    {
        list.erase(list.begin());
    }
    return list;
}

inline TargetArg ParseTargetArg(sol::object obj)
{
    TargetArg res;
    if (obj.is<uint32_t>() || obj.is<int>())
    {
        res.type = TargetArg::Type::Id;
        res.id = static_cast<uint32_t>(obj.as<int>());
    }
    else if (obj.is<AssetHandle>())
    {
        res.type = TargetArg::Type::Handle;
        res.handle = obj.as<AssetHandle>();
    }
    else if (obj.is<AssetHandle*>())
    {
        AssetHandle* ptr = obj.as<AssetHandle*>();
        if (ptr)
        {
            res.type = TargetArg::Type::Handle;
            res.handle = *ptr;
        }
    }
    else if (obj.is<std::string>() || obj.get_type() == sol::type::string)
    {
        res.type = TargetArg::Type::Path;
        res.path = obj.as<std::string>();
    }
    return res;
}

}

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

void LuaBindings::RegisterSceneAPI(sol::state& lua, SceneManager* sceneManager)
{
    lua["AsyncLoadStatus"] = lua.create_table_with(
        "None", static_cast<int>(SceneManager::AsyncLoadStatus::None),
        "Loading", static_cast<int>(SceneManager::AsyncLoadStatus::Loading),
        "Loaded", static_cast<int>(SceneManager::AsyncLoadStatus::Loaded),
        "Failed", static_cast<int>(SceneManager::AsyncLoadStatus::Failed),
        "Cancelled", static_cast<int>(SceneManager::AsyncLoadStatus::Cancelled)
    );

    lua["Scene"] = lua.create_table();
    lua["SceneManager"] = lua.create_table();

    lua["SceneManager"]["ChangeScene"] = [sceneManager](sol::variadic_args va) -> bool {
        if (!sceneManager)
        {
            LOG_ERROR("SceneManager:ChangeScene is unavailable outside an Engine-managed scene");
            return false;
        }

        auto args = GetCleanArgs(va);
        if (args.empty())
        {
            LOG_ERROR("SceneManager:ChangeScene expects a .rscene path, AssetHandle, or taskId");
            return false;
        }

        TargetArg target = ParseTargetArg(args[0]);
        if (target.type == TargetArg::Type::Id)
            return sceneManager->ChangeToLoadedScene(target.id);
        if (target.type == TargetArg::Type::Handle)
            return sceneManager->LoadScene(target.handle);
        if (target.type == TargetArg::Type::Path)
            return sceneManager->LoadScene(target.path);

        LOG_ERROR("SceneManager:ChangeScene expects a .rscene path, AssetHandle, or taskId");
        return false;
    };

    lua["SceneManager"]["LoadSceneAsync"] = [sceneManager](sol::variadic_args va) -> uint32_t {
        if (!sceneManager)
        {
            LOG_ERROR("SceneManager:LoadSceneAsync is unavailable outside an Engine-managed scene");
            return 0;
        }

        auto args = GetCleanArgs(va);
        if (args.empty())
        {
            LOG_ERROR("SceneManager:LoadSceneAsync requires a scene path or AssetHandle");
            return 0;
        }

        bool autoActivate = false;
        if (args.size() > 1 && args[1].is<bool>())
        {
            autoActivate = args[1].as<bool>();
        }

        TargetArg target = ParseTargetArg(args[0]);
        if (target.type == TargetArg::Type::Handle)
            return sceneManager->LoadSceneAsync(target.handle, autoActivate);
        if (target.type == TargetArg::Type::Path)
            return sceneManager->LoadSceneAsync(target.path, autoActivate);

        LOG_ERROR("SceneManager:LoadSceneAsync expects a .rscene path or AssetHandle");
        return 0;
    };

    lua["SceneManager"]["ChangeSceneAsync"] = [sceneManager](sol::variadic_args va) -> uint32_t {
        if (!sceneManager)
        {
            LOG_ERROR("SceneManager:ChangeSceneAsync is unavailable outside an Engine-managed scene");
            return 0;
        }

        auto args = GetCleanArgs(va);
        if (args.empty())
        {
            LOG_ERROR("SceneManager:ChangeSceneAsync requires a scene path or AssetHandle");
            return 0;
        }

        TargetArg target = ParseTargetArg(args[0]);
        if (target.type == TargetArg::Type::Handle)
            return sceneManager->ChangeSceneAsync(target.handle);
        if (target.type == TargetArg::Type::Path)
            return sceneManager->ChangeSceneAsync(target.path);

        LOG_ERROR("SceneManager:ChangeSceneAsync expects a .rscene path or AssetHandle");
        return 0;
    };

    lua["SceneManager"]["UnloadUnusedAssets"] = []() {
        AssetManager::Get().UnloadUnusedAssets();
    };

    lua["SceneManager"]["IsSceneLoaded"] = [sceneManager](sol::variadic_args va) -> bool {
        if (!sceneManager) return false;
        auto args = GetCleanArgs(va);
        if (args.empty()) return false;

        TargetArg target = ParseTargetArg(args[0]);
        if (target.type == TargetArg::Type::Id)
            return sceneManager->IsSceneLoaded(target.id);
        if (target.type == TargetArg::Type::Handle)
            return sceneManager->IsSceneLoaded(target.handle);
        if (target.type == TargetArg::Type::Path)
            return sceneManager->IsSceneLoaded(target.path);

        return false;
    };

    lua["SceneManager"]["GetLoadProgress"] = [sceneManager](sol::variadic_args va) -> float {
        if (!sceneManager) return 0.0f;
        auto args = GetCleanArgs(va);
        if (args.empty()) return 0.0f;

        TargetArg target = ParseTargetArg(args[0]);
        if (target.type == TargetArg::Type::Id)
            return sceneManager->GetLoadProgress(target.id);
        if (target.type == TargetArg::Type::Handle)
            return sceneManager->GetLoadProgress(target.handle);
        if (target.type == TargetArg::Type::Path)
            return sceneManager->GetLoadProgress(target.path);

        return 0.0f;
    };

    lua["SceneManager"]["GetLoadStatus"] = [sceneManager](sol::variadic_args va) -> std::string {
        if (!sceneManager) return "None";
        auto args = GetCleanArgs(va);
        if (args.empty()) return "None";

        SceneManager::AsyncLoadStatus status = SceneManager::AsyncLoadStatus::None;
        TargetArg target = ParseTargetArg(args[0]);
        if (target.type == TargetArg::Type::Id)
            status = sceneManager->GetLoadStatus(target.id);
        else if (target.type == TargetArg::Type::Handle)
            status = sceneManager->GetLoadStatus(target.handle);
        else if (target.type == TargetArg::Type::Path)
            status = sceneManager->GetLoadStatus(target.path);

        switch (status)
        {
            case SceneManager::AsyncLoadStatus::Loading:   return "Loading";
            case SceneManager::AsyncLoadStatus::Loaded:    return "Loaded";
            case SceneManager::AsyncLoadStatus::Failed:    return "Failed";
            case SceneManager::AsyncLoadStatus::Cancelled: return "Cancelled";
            default:                                       return "None";
        }
    };

    lua["SceneManager"]["ChangeToLoadedScene"] = [sceneManager](sol::variadic_args va) -> bool {
        if (!sceneManager)
        {
            LOG_ERROR("SceneManager:ChangeToLoadedScene is unavailable outside an Engine-managed scene");
            return false;
        }

        auto args = GetCleanArgs(va);
        if (args.empty())
        {
            LOG_ERROR("SceneManager:ChangeToLoadedScene requires a taskId, .rscene path, or AssetHandle");
            return false;
        }

        TargetArg target = ParseTargetArg(args[0]);
        if (target.type == TargetArg::Type::Id)
            return sceneManager->ChangeToLoadedScene(target.id);
        if (target.type == TargetArg::Type::Handle)
            return sceneManager->ChangeToLoadedScene(target.handle);
        if (target.type == TargetArg::Type::Path)
            return sceneManager->ChangeToLoadedScene(target.path);

        LOG_ERROR("SceneManager:ChangeToLoadedScene expects a taskId, .rscene path, or AssetHandle");
        return false;
    };

    lua["SceneManager"]["UnloadLoadedScene"] = [sceneManager](sol::variadic_args va) -> bool {
        if (!sceneManager) return false;
        auto args = GetCleanArgs(va);
        if (args.empty()) return false;

        TargetArg target = ParseTargetArg(args[0]);
        if (target.type == TargetArg::Type::Id)
            return sceneManager->UnloadLoadedScene(target.id);
        if (target.type == TargetArg::Type::Handle)
            return sceneManager->UnloadLoadedScene(target.handle);
        if (target.type == TargetArg::Type::Path)
            return sceneManager->UnloadLoadedScene(target.path);

        return false;
    };

    lua["SceneManager"]["CancelAsyncLoad"] = [sceneManager](sol::variadic_args va) -> bool {
        if (!sceneManager) return false;
        auto args = GetCleanArgs(va);
        if (args.empty()) return false;

        TargetArg target = ParseTargetArg(args[0]);
        if (target.type == TargetArg::Type::Id)
            return sceneManager->CancelAsyncLoad(target.id);
        if (target.type == TargetArg::Type::Handle)
            return sceneManager->CancelAsyncLoad(target.handle);
        if (target.type == TargetArg::Type::Path)
            return sceneManager->CancelAsyncLoad(target.path);

        return false;
    };

    lua["SceneManager"]["GetLoadedScenes"] = [sceneManager](sol::this_state s, sol::variadic_args va) -> sol::table {
        sol::state_view luaState(s);
        sol::table result = luaState.create_table();
        if (!sceneManager) return result;

        auto paths = sceneManager->GetLoadedScenePaths();
        for (size_t i = 0; i < paths.size(); ++i)
        {
            result[i + 1] = paths[i];
        }
        return result;
    };

    lua["SceneManager"]["GetAutoload"] = [sceneManager](sol::this_state s, sol::variadic_args va) -> sol::object {
        if (!sceneManager) return sol::nil;
        auto args = GetCleanArgs(va);
        if (args.empty() || !args[0].is<std::string>()) return sol::nil;
        const std::string name = args[0].as<std::string>();

        Scene& scene = sceneManager->GetActiveScene();
        auto view = scene.GetRegistry().view<AutoloadComponent>();
        for (auto e : view)
        {
            if (view.get<AutoloadComponent>(e).name == name)
            {
                if (scene.HasComponent<ScriptComponent>(e))
                {
                    auto& sc = scene.GetComponent<ScriptComponent>(e);
                    if (sc.luaInstance.valid()) return sc.luaInstance;
                }
                return sol::make_object(s, Entity(e, &scene));
            }
        }
        return sol::nil;
    };

    lua.new_usertype<Scene>("Scene",
        "IsValid", [](Scene& scene) { return true; },
        "CreateEntity", &Scene::CreateEntity,
        "DestroyEntity", [](Scene& scene, Entity& e) { scene.QueueDestroyEntity(e); },
        "FindEntityByName", &Scene::GetEntityByName,
        "Instantiate", [](Scene& scene, sol::object prefabObj, sol::optional<sol::object> posObj, sol::optional<sol::object> rotObj, sol::optional<sol::object> parentObj) {
            return InstantiatePrefabHelper(scene, prefabObj, posObj, rotObj, parentObj);
        },
        "GetAutoload", [](Scene& scene, sol::this_state s, const std::string& name) -> sol::object {
            auto view = scene.GetRegistry().view<AutoloadComponent>();
            for (auto e : view)
            {
                if (view.get<AutoloadComponent>(e).name == name)
                {
                    if (scene.HasComponent<ScriptComponent>(e))
                    {
                        auto& sc = scene.GetComponent<ScriptComponent>(e);
                        if (sc.luaInstance.valid()) return sc.luaInstance;
                    }
                    return sol::make_object(s, Entity(e, &scene));
                }
            }
            return sol::nil;
        }
    );
}
