#include "Scene.h"
#include "../Core/Time.h"
#include <iostream>

Scene::Scene() : registry() {}

entt::entity Scene::createEntity(const std::string& name) {
    entt::entity entity = registry.create();
    addComponent<TransformComponent>(entity,glm::vec3(0.0f),glm::vec3(0.0f),glm::vec3(1.0f));
    return entity;
}

void Scene::destroyEntity(entt::entity entity) {
    registry.destroy(entity);
}

void Scene::update() {
    
}

entt::registry& Scene::getRegistry()
{
    return registry;
}
