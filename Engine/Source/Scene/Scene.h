#pragma once

#include <entt/entt.h>
#include "Math/RvelaMath.h"
#include "Utils/ISerializable.h"
#include "Renderer/Renderer.h"
#include "CameraSystem.h"
#include "Script/ScriptSystem.h"
#include "LightSystem.h"
#include "TransformSystem.h"
#include "Environment.h"
#include "Physics/PhysicsSystem.h"
#include "Audio/AudioSystem.h"
#include "ParticleSystem.h"

namespace rv {

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
    ~Scene();
    void SetState(SceneState newState);
    SceneState GetState() const { return m_State; };
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    inline const std::string& GetPath() noexcept { return m_ScenePath; }
    inline void SetPath(const std::string& path) noexcept { m_ScenePath = path; }

    void OnStart();
    void OnUpdate(float dt);
    void OnFixedUpdate(float dt);
    void OnLateUpdate(float dt);
    void OnStop();

    bool isLoading = false;

    Entity CreateEntity(const std::string& name);
    Entity CreateEntityRaw();
    Entity CreateEntityWithUUID(const std::string& name, EntityUUID uuid);
    void DestroyEntity(entt::entity entity);
    void DestroyEntity(Entity& entity);
    void QueueDestroyEntity(Entity& entity);
    void QueueDestroyEntity(entt::entity entity);

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

    void Update();
    void FixedUpdate();
    void LateUpdate();

    Entity Instantiate(const AssetUUID& prefabUUID);

    entt::registry& GetRegistry();
    std::unordered_map<EntityUUID, entt::entity>& GetUUIDEntityMap() { return m_EntityMap; }

    [[nodiscard]] inline const entt::entity& GetRootEntity() const noexcept { return m_RootEntity; }

    entt::entity GetEntityByUUID(EntityUUID& uuid) { return m_EntityMap.at(uuid); }
    Entity GetEntityByName(const std::string& name);

    void SetSelectedEntity(entt::entity selectedEntity) { this->selectedEntity = selectedEntity; }
    entt::entity GetSelectedEntity() { return selectedEntity; }

    [[nodiscard]] inline std::vector<entt::entity> GetRootEntities() noexcept
    {
        std::vector<entt::entity> roots;

        if (!m_Registry.valid(entt::null)) 
        { 
            return roots;
        }

        auto view = m_Registry.view<SceneTreeComponent>();
        for (auto e : view) {
            const auto& stc = view.get<SceneTreeComponent>(e);
            if (stc.parent == entt::null) 
            {
                roots.push_back(e);
            }
        }
        return roots;
    }

    inline TransformSystem& GetTransformSystem() noexcept { return m_TransformSystem; }
    inline CameraSystem& GetCameraSystem() noexcept { return m_CameraSystem; }
    inline ScriptSystem& GetScriptSystem() noexcept { return m_ScriptSystem; }
    inline LightSystem& GetLightSystem() noexcept { return m_LightSystem; }
    inline PhysicsSystem& GetPhysicsSystem() noexcept { return m_PhysicsSystem; }
    inline Environment& GetEnvironment() noexcept { return m_Environment; }


private:
    void FlushDestroyQueue()
    {
        for (entt::entity e : m_pendingDestroys)
            DestroyEntity(e);
        m_pendingDestroys.clear();
    }

private:
    bool m_isUpdating = false;
    std::vector<entt::entity> m_pendingDestroys;

    unsigned int CountEntitiesRecursively(entt::entity& rootEntity);
    friend class Entity;
    SceneState m_State = SceneState::EDIT;
    std::string m_SceneName;
    std::string m_ScenePath;
    std::unordered_map<EntityUUID, entt::entity> m_EntityMap;
    entt::entity selectedEntity;
    entt::entity m_RootEntity;
    TransformSystem m_TransformSystem;
    CameraSystem m_CameraSystem;
    ScriptSystem m_ScriptSystem;
    LightSystem m_LightSystem;
    entt::registry m_Registry;
    PhysicsSystem m_PhysicsSystem;
    AudioSystem m_AudioSystem;
    ParticleSystem m_ParticleSystem;

    Environment m_Environment;
};

}