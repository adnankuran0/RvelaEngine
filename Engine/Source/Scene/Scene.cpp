#include "Scene.h"
#include "Entity.h"
#include "../Core/Time.h"
#include <iostream>



Scene::Scene() : m_Registry() {}

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(m_Registry.create(),this);
    
    entity.AddComponent<TransformComponent>(glm::vec3(0.0f),glm::vec3(0.0f),glm::vec3(1.0f));
    entity.AddComponent<TagComponent>(name);
    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    m_Registry.destroy(entity);
}



void Scene::Update() {
    
}

entt::registry& Scene::GetRegistry()
{
    return m_Registry;
}
