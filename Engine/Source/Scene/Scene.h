#pragma once

#include <entt/entt.h>

#include "Components.h"
#include "Math/RvelaMath.h"
#include "Utils/ISerializable.h"
#include "Renderer/Renderer.h"


class Entity;

class Scene 
{
public:
    Scene();

    bool isLoading = false;

    Entity CreateEntity(const std::string& name);
    Entity CreateEntityWithUUID(const std::string& name, EntityUUID uuid);
    void DestroyEntity(entt::entity entity);

    Entity CreatePointLight();
    Entity CreateDirectionalLight();

    Entity LoadAsset(const std::string& path);
    Entity LoadPrimitive(const std::string& primitiveMeshName);

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
        const glm::mat4& parentWorld);

    void Update();

    entt::registry& GetRegistry();
    std::unordered_map<EntityUUID, entt::entity> GetUUIDEntityMap() { return m_EntityMap; }

    std::vector<PointLightData> CollectPointLights() noexcept;
    std::optional<DirectionalLightData>  CollectDirectionalLight() noexcept;

    void SetSelectedEntity(entt::entity selectedEntity) { this->selectedEntity = selectedEntity; }
    entt::entity GetSelectedEntity() { return selectedEntity; }

private:
    entt::registry m_Registry;
    std::unordered_map<EntityUUID, entt::entity> m_EntityMap;
    friend class Entity;
    entt::entity selectedEntity;
};
