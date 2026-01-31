#pragma once

#include <entt/entt.h>

#include "Components.h"
#include "Math/RvelaMath.h"
#include "Utils/ISerializable.h"
#include "Rendering/Renderer.h"
#include "CameraSystem.h"

class Entity; // Forward Declaration

enum class SceneState
{
    EDIT,
    PLAY,
    PAUSE
};

class Scene
{
public:
    Scene(const std::string& sceneName = "Untitled");

    void SetState(SceneState newState);
    SceneState GetState() const { return m_State; };

    void BindLuaScript(ScriptComponent& sc, entt::entity& e);
    

    void OnStart();
    void OnUpdate(float dt);
    void OnStop();;

    bool isLoading = false;

    Entity CreateEntity(const std::string& name);
    Entity CreateEntityRaw();
    Entity CreateEntityWithUUID(const std::string& name, EntityUUID uuid);
    void DestroyEntity(entt::entity entity);

    Entity CreatePointLight();
    Entity CreateDirectionalLight();

    Entity LoadPrimitive(const std::string& primitiveMeshName);

    template<typename Component, typename... Args>
    Component& AddComponent(entt::entity entity, Args&&... args) {
        return m_Registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    Component& GetComponent(entt::entity entity) {
        return m_Registry.get<Component>(entity);
    }

    template<typename Component>
    bool HasComponent(entt::entity entity) {
        return m_Registry.any_of<Component>(entity);
    }

    template<typename Component>
    bool HasComponent(Entity entity) {
        return m_Registry.any_of<Component>(static_cast<entt::entity>(entity));
    }

    template<typename Component>
    void RemoveComponent(entt::entity entity) {
        m_Registry.remove<Component>(entity);
    }

    void SetParent(entt::entity child, entt::entity parent);
    void SetParentKeepLocal(entt::entity child, entt::entity parent);

    const std::vector<entt::entity>& GetChildren(entt::entity entity) {
        return GetComponent<SceneTreeComponent>(entity).children;
    }

    void RemoveParent(entt::entity child);

    entt::entity GetParent(entt::entity entity) {
        if (HasComponent<SceneTreeComponent>(entity))
            return GetComponent<SceneTreeComponent>(entity).parent;
        return entt::null;
    }

    void UpdateHierarchy();

    void UpdateNodeRecursive(entt::entity entity,
        const glm::mat4& parentWorld);

    void Update();

    Entity Instantiate(const AssetUUID& prefabUUID);

    entt::registry& GetRegistry();
    std::unordered_map<EntityUUID, entt::entity> GetUUIDEntityMap() { return m_EntityMap; }

    [[nodiscard]] inline const entt::entity& GetRootEntity() const noexcept { return m_RootEntity; }

    entt::entity& GetEntityByUUID(EntityUUID& uuid) { return m_EntityMap.at(uuid); }

    std::vector<PointLightData> CollectPointLights() noexcept;
    std::optional<DirectionalLightData>  CollectDirectionalLight() noexcept;

    void SetSelectedEntity(entt::entity selectedEntity) { this->selectedEntity = selectedEntity; }
    entt::entity GetSelectedEntity() { return selectedEntity; }

    [[nodiscard]] inline std::vector<entt::entity> GetRootEntities() noexcept
    {
        std::vector<entt::entity> roots;
        m_Registry.view<SceneTreeComponent>().each([&](entt::entity e, SceneTreeComponent& stc) {
            if (stc.parent == entt::null) {
                roots.push_back(e);
            }
            });
        return roots;
    }

    [[nodiscard]] unsigned int GetComponentCount(entt::entity entity) noexcept;

    inline CameraSystem& GetCameraSystem() noexcept { return m_CameraSystem; }

private:
    friend class Entity;
    SceneState m_State = SceneState::EDIT;
    std::string m_SceneName;
    entt::registry m_Registry;
    std::unordered_map<EntityUUID, entt::entity> m_EntityMap;
    entt::entity selectedEntity;
    entt::entity m_RootEntity;
    sol::state lua;
    CameraSystem m_CameraSystem;
};
