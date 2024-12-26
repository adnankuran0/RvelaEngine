#include "Scene.h"
#include "Entity.h"
#include "../Core/Time.h"
#include <iostream>
#include "RvelaLog.h"
#include "../Resources/cube.h"



Scene::Scene() : m_Registry() {}

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(m_Registry.create(),this);
    
    entity.AddComponent<TransformComponent>(glm::vec3(0.0f),glm::vec3(0.0f),glm::vec3(1.0f));
    entity.AddComponent<WorldTransformComponent>(glm::vec3(0.0f),glm::vec3(0.0f),glm::vec3(1.0f));
    entity.AddComponent<SceneTreeComponent>();
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<MeshComponent>(vertices, sizeof(vertices), indices, sizeof(indices));
    entity.AddComponent<MaterialComponent>("D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/vertex.glsl",
        "D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/fragment.glsl", "D:/GitHub/RvelaEngine/Engine/Source/Resources/Textures/metal");
    return entity;
}

void Scene::DestroyEntity(entt::entity entity) {

    GetComponent<MeshComponent>(entity).Destroy();
    GetComponent<MaterialComponent>(entity).Destroy();
    m_Registry.destroy(entity);
}



void Scene::Update() {
    UpdateHierarchy();
}

entt::registry& Scene::GetRegistry()
{
    return m_Registry;
}

void Scene::UpdateHierarchy()
{
    auto view = m_Registry.view<SceneTreeComponent, TransformComponent,WorldTransformComponent>();
    for (auto entity : view) {
        auto& node = m_Registry.get<SceneTreeComponent>(entity);
        auto& transform = m_Registry.get<TransformComponent>(entity);
        auto& worldTransform = m_Registry.get<WorldTransformComponent>(entity);
        if (node.parent != entt::null && m_Registry.valid(node.parent) && m_Registry.any_of<TransformComponent>(node.parent) && m_Registry.any_of<WorldTransformComponent>(node.parent)) {
            auto& parentTransform = m_Registry.get<TransformComponent>(node.parent);
            glm::mat4 parentMatrix = parentTransform.GetMatrix();
            glm::mat4 localMatrix = transform.GetMatrix();

            glm::mat4 worldMatrix =  parentMatrix * localMatrix;
            glm::vec3 worldPosition = glm::vec3(worldMatrix[3]);

            glm::vec3 scale, rotationEuler, translation;
            DecomposeToEulerAngles(worldMatrix, scale, rotationEuler, translation);


            worldTransform.scale = scale;
            worldTransform.rotation = rotationEuler;
            worldTransform.position = translation;
        }
        else {
            worldTransform.scale = transform.scale;
            worldTransform.rotation = transform.rotation;
            worldTransform.position = transform.position;
        }
    }
}