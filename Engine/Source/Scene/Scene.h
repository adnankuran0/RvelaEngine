#pragma once

#include <entt/entt.h>

#include "Components.h"
#include "Core/RvelaMath.h"
#include "Core/Utils/ISerializable.h"

class Entity;

class Scene 
{
public:
    Scene();

    Entity CreateEntity(const std::string& name);
    Entity CreateEntityWithUUID(const std::string& name, UUID uuid);
    void DestroyEntity(entt::entity entity);

    Entity LoadAsset(const std::string& path);

    template<typename Component, typename... Args>
    void AddComponent(entt::entity entity, Args&&... args) {
        m_Registry.emplace<Component>(entity, std::forward<Args>(args)...);
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
    void RemoveComponent(entt::entity entity) {
        m_Registry.remove<Component>(entity);
    }

    void SetParent(entt::entity child, entt::entity parent); 

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
        const glm::vec3& parentPos,
        const glm::quat& parentRot,
        const glm::vec3& parentScale);

    void Update();

    entt::registry& GetRegistry();
    std::unordered_map<UUID, entt::entity> GetUUIDEntityMap() { return m_EntityMap; }

private:
    entt::registry m_Registry;
    std::unordered_map<UUID, entt::entity> m_EntityMap;
    friend class Entity;
};
