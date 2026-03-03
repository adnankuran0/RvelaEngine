#include "rvelapch.h"
#include "TransformSystem.h"
#include "Scene/Scene.h"

using namespace rv;



void TransformSystem::Update() 
{
    auto view = m_Scene.GetRegistry().view<SceneTreeComponent, TransformComponent>();
    UpdateNodeRecursive(m_Scene.GetRootEntity(), glm::mat4(1.0f),true);
}

void TransformSystem::UpdateNodeRecursive(entt::entity e, const glm::mat4& parentWorldMatrix, bool parentDirty)
{
    auto& transform = m_Scene.GetComponent<TransformComponent>(e);

    bool isDirty = transform.IsDirty() || parentDirty;

    glm::mat4 worldMatrix;
    if (isDirty) {
        worldMatrix = parentWorldMatrix * transform.GetLocalMatrix();
        transform.SetWorldMatrix(worldMatrix);
    }
    else {
        worldMatrix = transform.GetWorldMatrix();
    }

    if (isDirty && m_Scene.HasComponent<MeshRendererComponent>(e)) {
        auto& meshRenderer = m_Scene.GetComponent<MeshRendererComponent>(e);
        meshRenderer.worldAABB = meshRenderer.localAABB.CalculateWorldAABB(worldMatrix);
    }

    auto& sceneTree = m_Scene.GetComponent<SceneTreeComponent>(e);
    for (auto child : sceneTree.children) {
        if (m_Scene.GetRegistry().valid(child))
            UpdateNodeRecursive(child, worldMatrix, isDirty);
    }
}