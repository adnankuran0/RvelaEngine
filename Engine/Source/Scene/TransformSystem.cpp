#include "rvelapch.h"
#include "TransformSystem.h"
#include "Scene/Scene.h"

using namespace rv;



void TransformSystem::Update()
{
    if (m_Scene.GetState() == SceneState::PLAY)
        InterpolatePhysicsBodies();

    UpdateNodeRecursive(m_Scene.GetRootEntity(), glm::mat4(1.0f), true);
}

void rv::TransformSystem::InterpolatePhysicsBodies()
{
    float alpha = Time::GetInterpolationAlpha();
    InterpolateRigidbodies(alpha);
    InterpolateCharacterBodies(alpha);
}

void rv::TransformSystem::InterpolateRigidbodies(float alpha)
{
    auto rbView = m_Scene.GetRegistry().view<TransformComponent, RigidbodyComponent, SceneTreeComponent>();
    for (auto e : rbView)
    {
        auto& rb = m_Scene.GetComponent<RigidbodyComponent>(e);
        if (!rb.interpolationReady || rb.bodyType == Physics::MotionType::STATIC) continue;

        auto& transform = m_Scene.GetComponent<TransformComponent>(e);
        auto& node = m_Scene.GetComponent<SceneTreeComponent>(e);

        glm::vec3 worldPos = glm::mix(rb.previousPosition, rb.currentPosition, alpha);
        glm::quat worldRot = glm::slerp(rb.previousRotation, rb.currentRotation, alpha);

        if (node.parent == entt::null || node.parent == m_Scene.GetRootEntity())
        {
            transform.SetPosition(worldPos);
            transform.SetRotation(worldRot);
        }
        else
        {
            auto& parentTransform = m_Scene.GetComponent<TransformComponent>(node.parent);
            glm::mat4 invParentWorld = glm::inverse(parentTransform.GetWorldMatrix());
            transform.SetPosition(glm::vec3(invParentWorld * glm::vec4(worldPos, 1.0f)));
            transform.SetRotation(glm::inverse(parentTransform.GetWorldRotation()) * worldRot);
        }
    }
}

void rv::TransformSystem::InterpolateCharacterBodies(float alpha)
{
    auto cbView = m_Scene.GetRegistry().view<TransformComponent, CharacterBodyComponent>();
    for (auto e : cbView)
    {
        auto& cb = m_Scene.GetComponent<CharacterBodyComponent>(e);
        if (!cb.interpolationReady || !cb.character) continue;

        auto& transform = m_Scene.GetComponent<TransformComponent>(e);
        transform.SetPosition(glm::mix(cb.previousPosition, cb.currentPosition, alpha));
        transform.SetRotation(glm::slerp(cb.previousRotation, cb.currentRotation, alpha));
    }
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