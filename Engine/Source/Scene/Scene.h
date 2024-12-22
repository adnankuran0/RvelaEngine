#pragma once

#include <entt/entt.h>
#include <string>
#include "Components.h"



class Scene {
public:
    Scene();

    entt::entity createEntity(const std::string& name);
    void destroyEntity(entt::entity entity);

    template<typename Component, typename... Args>
    void addComponent(entt::entity entity, Args&&... args) {
        m_Registry.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template<typename Component>
    Component& getComponent(entt::entity entity) {
        return m_Registry.get<Component>(entity);
    }

    template<typename Component>
    bool hasComponent(entt::entity entity) {
        return m_Registry.any_of<Component>(entity);
    }

    template<typename Component>
    void removeComponent(entt::entity entity) {
        m_Registry.remove<Component>(entity);
    }

    void update();

    entt::registry& getRegistry();

private:
    entt::registry m_Registry;
    friend class Entity;
};
