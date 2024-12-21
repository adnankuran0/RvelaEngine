#pragma once
#include "Entity.h"
#include <unordered_set>

class EntityManager {
public:
    Entity createEntity() {
        Entity entity(nextEntityID++);
        activeEntities.insert(entity);
        return entity;
    }

    void destroyEntity(Entity entity) {
        activeEntities.erase(entity);
    }

    bool isAlive(Entity entity) const {
        return activeEntities.find(entity) != activeEntities.end();
    }

private:
    uint32_t nextEntityID = 0;
    std::unordered_set<Entity> activeEntities;
};
