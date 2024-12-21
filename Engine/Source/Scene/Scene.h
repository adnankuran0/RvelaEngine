#pragma once
#include "EntityManager.h"
#include "ComponentManager.h"
#include "System.h"
#include <vector>
#include <memory>

class Scene {
public:
    Entity createEntity() {
        return entityManager.createEntity();
    }

    void destroyEntity(Entity entity) {
        entityManager.destroyEntity(entity);
        // Component temizleme yapýlabilir.
    }

    template<typename T, typename... Args>
    void addComponent(Entity entity, Args&&... args) {
        componentManager.addComponent<T>(entity, T(std::forward<Args>(args)...));
    }

    template<typename T>
    T& getComponent(Entity entity) {
        return componentManager.getComponent<T>(entity);
    }

    template<typename T>
    bool hasComponent(Entity entity) {
        return componentManager.hasComponent<T>(entity);
    }

    void addSystem(std::unique_ptr<System> system) {
        systems.emplace_back(std::move(system));
    }

    void update(float deltaTime) {
        for (auto& system : systems) {
            system->update(*this, deltaTime);
        }
    }

private:
    EntityManager entityManager;
    ComponentManager componentManager;
    std::vector<std::unique_ptr<System>> systems;
};
