#include "rvelapch.h"
#include "Scene.h"
#include "Entity.h"
#include "../Core/Time.h"
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
        "D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/fragment.glsl","D:/GitHub/RvelaEngine/Engine/Source/Resources/Textures/metal");
    return entity;
}

void Scene::DestroyEntity(entt::entity entity) {
    
    if (HasComponent<MeshComponent>(entity))
        GetComponent<MeshComponent>(entity).Destroy();
    if (HasComponent<MaterialComponent>(entity))
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

void Scene::UpdateHierarchy() {
    auto view = m_Registry.view<SceneTreeComponent, TransformComponent, WorldTransformComponent>();

    std::vector<entt::entity> rootEntities;
    for (auto entity : view) {
        auto& node = m_Registry.get<SceneTreeComponent>(entity);
        if (node.parent == entt::null || !m_Registry.valid(node.parent)) {
            rootEntities.push_back(entity);
        }
    }

    for (auto root : rootEntities) {
        UpdateNodeRecursive(root, { 0, 0, 0 }, glm::quat(1, 0, 0, 0), { 1, 1, 1 });
    }
}

void Scene::UpdateNodeRecursive(entt::entity entity,
    const glm::vec3& parentPos,
    const glm::quat& parentRot,
    const glm::vec3& parentScale) {
    if (!m_Registry.valid(entity)) return;

    auto& transform = m_Registry.get<TransformComponent>(entity);
    auto& worldTransform = m_Registry.get<WorldTransformComponent>(entity);

    glm::quat localRot = transform.GetQuaternion();
    glm::quat worldRot = parentRot * localRot;

    glm::vec3 scaledPos = transform.position * parentScale;
    glm::vec3 worldPos = parentPos + parentRot * scaledPos;

    glm::vec3 worldScale = parentScale * transform.scale;

    worldTransform.position = worldPos;
    worldTransform.rotation = glm::degrees(glm::eulerAngles(worldRot));
    worldTransform.scale = worldScale;

    auto& node = m_Registry.get<SceneTreeComponent>(entity);
    for (auto child : node.children) {
        UpdateNodeRecursive(child, worldPos, worldRot, worldScale);
    }
}

void Scene::SetParent(entt::entity child, entt::entity parent) {
    if (!HasComponent<SceneTreeComponent>(child))
        AddComponent<SceneTreeComponent>(child);
    if (parent != entt::null && !HasComponent<SceneTreeComponent>(parent))
        AddComponent<SceneTreeComponent>(parent);

    auto& childNode = GetComponent<SceneTreeComponent>(child);

    glm::mat4 childWorldMatrix;
    if (HasComponent<WorldTransformComponent>(child)) {
        auto& childWorldTransform = GetComponent<WorldTransformComponent>(child);
        childWorldMatrix = childWorldTransform.GetMatrix();
    }
    else {
        auto& childTransform = GetComponent<TransformComponent>(child);
        childWorldMatrix = childTransform.GetMatrix();
    }

    if (childNode.parent != entt::null) {
        auto& oldParentNode = GetComponent<SceneTreeComponent>(childNode.parent);
        oldParentNode.children.erase(
            std::remove(oldParentNode.children.begin(), oldParentNode.children.end(), child),
            oldParentNode.children.end()
        );
    }

    childNode.parent = parent;

    if (parent != entt::null) {
        auto& parentNode = GetComponent<SceneTreeComponent>(parent);
        parentNode.children.push_back(child);

        auto& parentWorldTransform = GetComponent<WorldTransformComponent>(parent);
        glm::mat4 parentWorldMatrix = parentWorldTransform.GetMatrix();
        glm::mat4 parentInverseMatrix = glm::inverse(parentWorldMatrix);

        glm::mat4 newLocalMatrix = parentInverseMatrix * childWorldMatrix;

        glm::vec3 newScale, newRotation, newPosition;
        DecomposeToEulerAngles(newLocalMatrix, newScale, newRotation, newPosition);

        auto& childTransform = GetComponent<TransformComponent>(child);
        childTransform.scale = newScale;
        childTransform.rotation = newRotation;
        childTransform.position = newPosition;
    }
    else {
        glm::vec3 scale, rotation, position;
        DecomposeToEulerAngles(childWorldMatrix, scale, rotation, position);

        auto& childTransform = GetComponent<TransformComponent>(child);
        childTransform.scale = scale;
        childTransform.rotation = rotation;
        childTransform.position = position;
    }
}

void Scene::RemoveParent(entt::entity child) {
    if (!HasComponent<SceneTreeComponent>(child))
        return;

    auto& childNode = GetComponent<SceneTreeComponent>(child);

    glm::mat4 childWorldMatrix(1.0f);
    if (HasComponent<WorldTransformComponent>(child)) {
        auto& childWorldTransform = GetComponent<WorldTransformComponent>(child);
        childWorldMatrix = childWorldTransform.GetMatrix();
    }
    else {
        auto& childTransform = GetComponent<TransformComponent>(child);
        childWorldMatrix = childTransform.GetMatrix();
    }

    if (childNode.parent != entt::null) {
        auto& parentNode = GetComponent<SceneTreeComponent>(childNode.parent);
        parentNode.children.erase(
            std::remove(parentNode.children.begin(), parentNode.children.end(), child),
            parentNode.children.end()
        );
    }
    childNode.parent = entt::null;

    glm::vec3 newScale, newRotation, newPosition;
    DecomposeToEulerAngles(childWorldMatrix, newScale, newRotation, newPosition);

    auto& childTransform = GetComponent<TransformComponent>(child);
    childTransform.position = newPosition;
    childTransform.rotation = newRotation;
    childTransform.scale = newScale;
}