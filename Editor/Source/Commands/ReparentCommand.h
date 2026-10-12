#pragma once
#include "Command.h"
#include "Scene/Scene.h"
#include "Scene/EntityUUID.h"
#include <string>

namespace rv {

class ReparentCommand : public Command
{
public:
    ReparentCommand(Scene* scene, entt::entity child, entt::entity newParent)
        : m_Scene(scene)
    {
        if (!m_Scene) return;

        auto& reg = m_Scene->GetRegistry();
        if (child != entt::null && reg.valid(child) && m_Scene->HasComponent<UUIDComponent>(child))
        {
            m_ChildUUID = m_Scene->GetComponent<UUIDComponent>(child).uuid;
            if (m_Scene->HasComponent<TransformComponent>(child))
            {
                auto& tc = m_Scene->GetComponent<TransformComponent>(child);
                m_OldLocalPos = tc.GetPosition();
                m_OldLocalRot = tc.GetRotation();
                m_OldLocalScale = tc.GetScale();
            }
        }

        entt::entity oldParent = m_Scene->GetParent(child);
        if (oldParent != entt::null && reg.valid(oldParent) && m_Scene->HasComponent<UUIDComponent>(oldParent))
        {
            m_OldParentUUID = m_Scene->GetComponent<UUIDComponent>(oldParent).uuid;
        }

        if (newParent != entt::null && reg.valid(newParent) && m_Scene->HasComponent<UUIDComponent>(newParent))
        {
            m_NewParentUUID = m_Scene->GetComponent<UUIDComponent>(newParent).uuid;
        }
    }

    void Execute() override
    {
        ApplyParent(m_NewParentUUID);
        if (!m_HasNewTransform && m_Scene && m_ChildUUID != 0)
        {
            auto it = m_Scene->GetUUIDEntityMap().find(m_ChildUUID);
            if (it != m_Scene->GetUUIDEntityMap().end() && m_Scene->GetRegistry().valid(it->second))
            {
                if (m_Scene->HasComponent<TransformComponent>(it->second))
                {
                    auto& tc = m_Scene->GetComponent<TransformComponent>(it->second);
                    m_NewLocalPos = tc.GetPosition();
                    m_NewLocalRot = tc.GetRotation();
                    m_NewLocalScale = tc.GetScale();
                    m_HasNewTransform = true;
                }
            }
        }
        else if (m_HasNewTransform && m_Scene && m_ChildUUID != 0)
        {
            auto it = m_Scene->GetUUIDEntityMap().find(m_ChildUUID);
            if (it != m_Scene->GetUUIDEntityMap().end() && m_Scene->GetRegistry().valid(it->second))
            {
                if (m_Scene->HasComponent<TransformComponent>(it->second))
                {
                    auto& tc = m_Scene->GetComponent<TransformComponent>(it->second);
                    tc.SetPosition(m_NewLocalPos);
                    tc.SetRotation(m_NewLocalRot);
                    tc.SetScale(m_NewLocalScale);
                }
            }
        }
    }

    void Undo() override
    {
        ApplyParent(m_OldParentUUID);
        if (m_Scene && m_ChildUUID != 0)
        {
            auto it = m_Scene->GetUUIDEntityMap().find(m_ChildUUID);
            if (it != m_Scene->GetUUIDEntityMap().end() && m_Scene->GetRegistry().valid(it->second))
            {
                if (m_Scene->HasComponent<TransformComponent>(it->second))
                {
                    auto& tc = m_Scene->GetComponent<TransformComponent>(it->second);
                    tc.SetPosition(m_OldLocalPos);
                    tc.SetRotation(m_OldLocalRot);
                    tc.SetScale(m_OldLocalScale);
                }
            }
        }
    }

    std::string GetName() const override
    {
        return "Reparent Entity";
    }

private:
    void ApplyParent(EntityUUID targetParentUUID)
    {
        if (!m_Scene || m_ChildUUID == 0) return;

        auto& uuidMap = m_Scene->GetUUIDEntityMap();
        auto itChild = uuidMap.find(m_ChildUUID);
        if (itChild == uuidMap.end() || !m_Scene->GetRegistry().valid(itChild->second))
            return;

        entt::entity childHandle = itChild->second;
        entt::entity parentHandle = m_Scene->GetRootEntity();

        if (targetParentUUID != 0)
        {
            auto itParent = uuidMap.find(targetParentUUID);
            if (itParent != uuidMap.end() && m_Scene->GetRegistry().valid(itParent->second))
            {
                parentHandle = itParent->second;
            }
        }

        m_Scene->SetParent(childHandle, parentHandle);
        m_Scene->SetDirty(true);
    }

    Scene* m_Scene = nullptr;
    EntityUUID m_ChildUUID = 0;
    EntityUUID m_OldParentUUID = 0;
    EntityUUID m_NewParentUUID = 0;
    glm::vec3 m_OldLocalPos{0.0f};
    glm::quat m_OldLocalRot{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 m_OldLocalScale{1.0f};
    glm::vec3 m_NewLocalPos{0.0f};
    glm::quat m_NewLocalRot{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 m_NewLocalScale{1.0f};
    bool m_HasNewTransform = false;
};

class ReorderEntityCommand : public Command
{
public:
    ReorderEntityCommand(Scene* scene, entt::entity source, entt::entity target, bool insertBefore)
        : m_Scene(scene), m_InsertBefore(insertBefore), m_FirstExecute(true)
    {
        if (!m_Scene || source == entt::null || target == entt::null) return;
        auto& reg = m_Scene->GetRegistry();
        if (!reg.valid(source) || !reg.valid(target)) return;

        if (m_Scene->HasComponent<UUIDComponent>(source))
            m_SourceUUID = m_Scene->GetComponent<UUIDComponent>(source).uuid;
        if (m_Scene->HasComponent<UUIDComponent>(target))
            m_TargetUUID = m_Scene->GetComponent<UUIDComponent>(target).uuid;

        entt::entity sourceParent = m_Scene->GetParent(source);
        if (sourceParent != entt::null && reg.valid(sourceParent) && m_Scene->HasComponent<UUIDComponent>(sourceParent))
            m_OldParentUUID = m_Scene->GetComponent<UUIDComponent>(sourceParent).uuid;

        entt::entity targetParent = m_Scene->GetParent(target);
        if (targetParent != entt::null && reg.valid(targetParent) && m_Scene->HasComponent<UUIDComponent>(targetParent))
            m_NewParentUUID = m_Scene->GetComponent<UUIDComponent>(targetParent).uuid;
        else
            m_NewParentUUID = m_OldParentUUID;

        if (sourceParent != entt::null && reg.valid(sourceParent) && m_Scene->HasComponent<SceneTreeComponent>(sourceParent))
            m_OldParentChildrenUUIDs = m_Scene->GetComponent<SceneTreeComponent>(sourceParent).childrenUUIDs;

        if (targetParent != entt::null && reg.valid(targetParent) && targetParent != sourceParent && m_Scene->HasComponent<SceneTreeComponent>(targetParent))
            m_NewParentChildrenUUIDsBefore = m_Scene->GetComponent<SceneTreeComponent>(targetParent).childrenUUIDs;
    }

    void Execute() override
    {
        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            CaptureNewOrder();
            return;
        }

        entt::entity source = GetEntity(m_SourceUUID);
        entt::entity target = GetEntity(m_TargetUUID);
        if (source != entt::null && target != entt::null)
        {
            m_Scene->MoveChildOrder(source, target, m_InsertBefore);
        }
    }

    void Undo() override
    {
        if (!m_Scene || m_SourceUUID == 0) return;

        entt::entity sourceHandle = GetEntity(m_SourceUUID);
        entt::entity oldParentHandle = GetEntity(m_OldParentUUID);

        if (m_OldParentUUID != m_NewParentUUID && sourceHandle != entt::null && oldParentHandle != entt::null)
        {
            m_Scene->SetParent(sourceHandle, oldParentHandle);
        }

        ApplyOrder(m_OldParentUUID, m_OldParentChildrenUUIDs);

        if (m_NewParentUUID != m_OldParentUUID)
        {
            ApplyOrder(m_NewParentUUID, m_NewParentChildrenUUIDsBefore);
        }
    }

    std::string GetName() const override
    {
        return "Reorder Entity";
    }

private:
    void CaptureNewOrder()
    {
        entt::entity parentHandle = GetEntity(m_NewParentUUID);
        if (parentHandle != entt::null && m_Scene->HasComponent<SceneTreeComponent>(parentHandle))
        {
            m_NewParentChildrenUUIDsAfter = m_Scene->GetComponent<SceneTreeComponent>(parentHandle).childrenUUIDs;
        }
    }

    void ApplyOrder(EntityUUID parentUUID, const std::vector<EntityUUID>& order)
    {
        if (!m_Scene || order.empty()) return;
        entt::entity parentHandle = GetEntity(parentUUID);
        if (parentHandle == entt::null || !m_Scene->HasComponent<SceneTreeComponent>(parentHandle)) return;

        auto& parentTree = m_Scene->GetComponent<SceneTreeComponent>(parentHandle);
        auto& uuidMap = m_Scene->GetUUIDEntityMap();
        auto& reg = m_Scene->GetRegistry();

        std::vector<entt::entity> newChildren;
        std::vector<EntityUUID> newUUIDs;
        for (auto u : order)
        {
            auto it = uuidMap.find(u);
            if (it != uuidMap.end() && reg.valid(it->second))
            {
                newChildren.push_back(it->second);
                newUUIDs.push_back(u);
            }
        }

        parentTree.children = newChildren;
        parentTree.childrenUUIDs = newUUIDs;
        m_Scene->SetDirty(true);
    }

    entt::entity GetEntity(EntityUUID uuid) const
    {
        if (!m_Scene) return entt::null;
        if (uuid == 0) return m_Scene->GetRootEntity();
        auto it = m_Scene->GetUUIDEntityMap().find(uuid);
        if (it != m_Scene->GetUUIDEntityMap().end() && m_Scene->GetRegistry().valid(it->second))
            return it->second;
        return entt::null;
    }

    Scene* m_Scene = nullptr;
    EntityUUID m_SourceUUID = 0;
    EntityUUID m_TargetUUID = 0;
    EntityUUID m_OldParentUUID = 0;
    EntityUUID m_NewParentUUID = 0;
    bool m_InsertBefore = true;
    std::vector<EntityUUID> m_OldParentChildrenUUIDs;
    std::vector<EntityUUID> m_NewParentChildrenUUIDsBefore;
    std::vector<EntityUUID> m_NewParentChildrenUUIDsAfter;
    bool m_FirstExecute = false;
};

}
