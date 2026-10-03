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

    if (isDirty && m_Scene.HasComponent<SkeletalMeshRendererComponent>(e)) {
        auto& meshRenderer = m_Scene.GetComponent<SkeletalMeshRendererComponent>(e);

        entt::entity skelEntity = FindSkeletonEntity(e);

        if (skelEntity != entt::null) {
            auto& skel = m_Scene.GetComponent<SkeletonComponent>(skelEntity);

            if (!skel.modelSpaceMatrices.empty()) {
                glm::vec3 boneMin(FLT_MAX);
                glm::vec3 boneMax(-FLT_MAX);

                for (const auto& boneMatrix : skel.modelSpaceMatrices) {
                    glm::vec3 bonePos = glm::vec3(boneMatrix[3]);
                    boneMin = glm::min(boneMin, bonePos);
                    boneMax = glm::max(boneMax, bonePos);
                }

                glm::vec3 padding = (meshRenderer.localAABB.max - meshRenderer.localAABB.min) * 0.2f;
                boneMin -= padding;
                boneMax += padding;

                AABB animatedAABB{ boneMin, boneMax };

                auto& skelTransform = m_Scene.GetComponent<TransformComponent>(skelEntity);
                meshRenderer.worldAABB = animatedAABB.CalculateWorldAABB(skelTransform.GetWorldMatrix());
            }
            else {
                meshRenderer.worldAABB = meshRenderer.localAABB.CalculateWorldAABB(worldMatrix);
            }
        }
        else {
            meshRenderer.worldAABB = meshRenderer.localAABB.CalculateWorldAABB(worldMatrix);
        }
    }

    auto& sceneTree = m_Scene.GetComponent<SceneTreeComponent>(e);
    for (auto child : sceneTree.children) {
        if (m_Scene.GetRegistry().valid(child))
            UpdateNodeRecursive(child, worldMatrix, isDirty);
    }
}

entt::entity TransformSystem::FindSkeletonEntity(entt::entity e)
{
    auto& reg = m_Scene.GetRegistry();
    entt::entity current = e;

    while (reg.valid(current))
    {
        if (reg.any_of<SkeletonComponent>(current))
            return current;

        if (!reg.any_of<SceneTreeComponent>(current))
            break;

        current = reg.get<SceneTreeComponent>(current).parent;
    }

    return entt::null;
}

void TransformSystem::SetParent(entt::entity child, entt::entity parent)
{
    auto& reg = m_Scene.GetRegistry();
    if (child == entt::null || !reg.valid(child)) return;
    if (child == parent) return;

    if (!m_Scene.HasComponent<SceneTreeComponent>(child))
        m_Scene.AddComponent<SceneTreeComponent>(child);

    auto& childNode = m_Scene.GetComponent<SceneTreeComponent>(child);
    if (childNode.parent == parent) return;

    if (parent != entt::null)
    {
        entt::entity curr = parent;
        while (curr != entt::null && reg.valid(curr))
        {
            if (curr == child)
            {
                return;
            }
            if (!m_Scene.HasComponent<SceneTreeComponent>(curr))
                break;
            curr = m_Scene.GetComponent<SceneTreeComponent>(curr).parent;
        }
    }

    if (parent != entt::null && !m_Scene.HasComponent<SceneTreeComponent>(parent))
        m_Scene.AddComponent<SceneTreeComponent>(parent);

    EntityUUID childUUID = m_Scene.GetComponent<UUIDComponent>(child).uuid;
    glm::mat4 childWorldMatrix = m_Scene.GetComponent<TransformComponent>(child).GetWorldMatrix();

    if (childNode.parent != entt::null && reg.valid(childNode.parent))
    {
        auto& oldParentNode = m_Scene.GetComponent<SceneTreeComponent>(childNode.parent);

        auto it = std::find(oldParentNode.children.begin(), oldParentNode.children.end(), child);
        if (it != oldParentNode.children.end())
            oldParentNode.children.erase(it);

        auto uuidIt = std::find(oldParentNode.childrenUUIDs.begin(),
            oldParentNode.childrenUUIDs.end(), childUUID);
        if (uuidIt != oldParentNode.childrenUUIDs.end())
            oldParentNode.childrenUUIDs.erase(uuidIt);

        m_Scene.GetComponent<TransformComponent>(childNode.parent).SetDirty();
    }

    childNode.parent = parent;
    childNode.parentUUID = (parent != entt::null) ? m_Scene.GetComponent<UUIDComponent>(parent).uuid : 0;

    if (parent != entt::null)
    {
        auto& parentNode = m_Scene.GetComponent<SceneTreeComponent>(parent);

        parentNode.children.push_back(child);

        if (std::find(parentNode.childrenUUIDs.begin(), parentNode.childrenUUIDs.end(), childUUID)
            == parentNode.childrenUUIDs.end())
        {
            parentNode.childrenUUIDs.push_back(childUUID);
        }

        glm::mat4 parentWorldMatrix = m_Scene.GetComponent<TransformComponent>(parent).GetWorldMatrix();
        glm::mat4 parentInverse = glm::inverse(parentWorldMatrix);

        glm::mat4 localMatrix = parentInverse * childWorldMatrix;

        glm::vec3 scale, translation, skew;
        glm::quat rotation;
        glm::vec4 perspective;
        glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);

        auto& childTransform = m_Scene.GetComponent<TransformComponent>(child);
        childTransform.SetPosition(translation);
        childTransform.SetRotation(rotation);
        childTransform.SetScale(scale);
        childTransform.SetDirty();

        m_Scene.GetComponent<TransformComponent>(parent).SetDirty();
    }
    else
    {
        glm::vec3 scale, euler, position;
        math::DecomposeToEulerAngles(childWorldMatrix, scale, euler, position);

        auto& childTransform = m_Scene.GetComponent<TransformComponent>(child);
        childTransform.SetPosition(position);
        childTransform.SetRotation(glm::quat(glm::radians(euler)));
        childTransform.SetScale(scale);
        childTransform.SetDirty();
    }
}

void TransformSystem::SetParentKeepLocal(entt::entity child, entt::entity parent)
{
    auto& reg = m_Scene.GetRegistry();
    if (child == entt::null || !reg.valid(child)) return;
    if (child == parent) return;

    auto& childTree = m_Scene.GetComponent<SceneTreeComponent>(child);
    EntityUUID childUUID = m_Scene.GetComponent<UUIDComponent>(child).uuid;

    if (childTree.parent != entt::null && reg.valid(childTree.parent))
    {
        auto& oldParentTree = m_Scene.GetComponent<SceneTreeComponent>(childTree.parent);
        std::erase(oldParentTree.children, child);
        std::erase(oldParentTree.childrenUUIDs, childUUID);
    }

    childTree.parent = parent;
    childTree.parentUUID = (parent != entt::null && reg.valid(parent))
        ? m_Scene.GetComponent<UUIDComponent>(parent).uuid
        : 0;

    if (parent != entt::null && reg.valid(parent))
    {
        auto& parentTree = m_Scene.GetComponent<SceneTreeComponent>(parent);
        if (std::find(parentTree.children.begin(), parentTree.children.end(), child) == parentTree.children.end())
        {
            parentTree.children.push_back(child);
            parentTree.childrenUUIDs.push_back(childUUID);
        }
    }
}

void TransformSystem::RemoveParent(entt::entity child)
{
    SetParent(child, m_Scene.GetRootEntity());
}