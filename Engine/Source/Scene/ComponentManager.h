#pragma once
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <cassert>

class ComponentManager {
public:
    template<typename T>
    void addComponent(Entity entity, T component) {
        auto& storage = getComponentStorage<T>();
        storage[entity.getID()] = std::make_shared<T>(std::move(component));
    }

    template<typename T>
    T& getComponent(Entity entity) {
        auto& storage = getComponentStorage<T>();
        assert(storage.find(entity.getID()) != storage.end() && "Component not found!");
        return *storage[entity.getID()];
    }

    template<typename T>
    bool hasComponent(Entity entity) {
        auto& storage = getComponentStorage<T>();
        return storage.find(entity.getID()) != storage.end();
    }

    template<typename T>
    void removeComponent(Entity entity) {
        auto& storage = getComponentStorage<T>();
        storage.erase(entity.getID());
    }

private:
    template<typename T>
    std::unordered_map<uint32_t, std::shared_ptr<T>>& getComponentStorage() {
        static std::unordered_map<uint32_t, std::shared_ptr<T>> storage;
        return storage;
    }
};
