#include "rvelapch.h"
#include "TransformSystem.h"
#include "Scene/Scene.h"

using namespace rv;



void TransformSystem::Update() 
{
    auto view = m_Scene.GetRegistry().view<SceneTreeComponent, TransformComponent>();

    UpdateNodeRecursive(m_Scene.GetRootEntity(), glm::mat4(1.0f));
    //if(GetComponent<TransformComponent>(root).IsDirty()) // child may be dirty
}

void TransformSystem::UpdateNodeRecursive(entt::entity e, const glm::mat4& parentWorld)
{
    auto& transform = m_Scene.GetComponent<TransformComponent>(e);

    glm::mat4 localMatrix = transform.GetLocalMatrix();
    glm::mat4 worldMatrix = parentWorld * localMatrix;

    glm::vec3 scale, skew, translation;
    glm::quat rotation;
    glm::vec4 perspective;
    glm::decompose(worldMatrix, scale, rotation, translation, skew, perspective);

    transform.SetWorldTransform(translation, rotation, scale);

    if (m_Scene.HasComponent<MeshRendererComponent>(e))
    {
        auto& meshRenderer = m_Scene.GetComponent<MeshRendererComponent>(e);
        meshRenderer.worldAABB = meshRenderer.localAABB.CalculateWorldAABB(worldMatrix);
    }

    //transform.ClearDirty();

    if (m_Scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& sceneTree = m_Scene.GetComponent<SceneTreeComponent>(e);
        for (auto child : sceneTree.children)
        {
            if (m_Scene.GetRegistry().valid(child))
            {
                UpdateNodeRecursive(child, worldMatrix);
            }
        }
    }
}