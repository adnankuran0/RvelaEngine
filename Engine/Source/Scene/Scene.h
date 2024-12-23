#pragma once

#include <entt/entt.h>
#include <string>
#include "Components.h"


class Entity;


class Scene {
public:
    Scene();

    Entity CreateEntity(const std::string& name);
    void DestroyEntity(Entity entity);

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




    void Update();

    entt::registry& GetRegistry();

private:
    entt::registry m_Registry;
    friend class Entity;
};
