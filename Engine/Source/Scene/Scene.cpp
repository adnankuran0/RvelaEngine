#include "Scene.h"
#include "../Core/Time.h"
#include <iostream>

Scene::Scene() : m_Registry() {}

entt::entity Scene::createEntity(const std::string& name) {
    entt::entity entity = m_Registry.create();
    addComponent<TransformComponent>(entity,glm::vec3(0.0f),glm::vec3(0.0f),glm::vec3(1.0f));
    addComponent<TagComponent>(entity, name);
    return entity;
}

void Scene::destroyEntity(entt::entity entity) {
    m_Registry.destroy(entity);
}

void Scene::update() {
    
}

entt::registry& Scene::getRegistry()
{
    return m_Registry;
}
