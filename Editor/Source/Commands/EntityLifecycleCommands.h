#pragma once
#include "Command.h"
#include "Scene/Scene.h"
#include "Scene/EntityUUID.h"
#include "Scene/SceneSerializer.h"
#include "EditorSelection.h"
#include <nlohmann/json.hpp>
#include <vector>

namespace rv {

struct EntitySnapshotRecord
{
    json entityData;
    EntityUUID parentUUID = 0;
    EntityUUID uuid = 0;
};

class CreateEntityCommand : public Command
{
public:
    CreateEntityCommand(Scene* scene, entt::entity existingEntity)
        : m_Scene(scene), m_FirstExecute(true)
    {
        if (!m_Scene || existingEntity == entt::null || !m_Scene->GetRegistry().valid(existingEntity))
            return;

        CollectSubtree(existingEntity);
    }

    void Execute() override
    {
        if (!m_Scene) return;

        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            return;
        }

        std::vector<entt::entity> restoredHandles;
        for (const auto& rec : m_Records)
        {
            Entity e = m_Scene->CreateEntityRaw();
            entt::entity handle = e.GetHandle();

            if (rec.entityData.contains("UUID"))
            {
                auto& uuidComp = m_Scene->GetComponent<UUIDComponent>(handle);
                uuidComp.Deserialize(rec.entityData["UUID"]);
                m_Scene->GetUUIDEntityMap()[uuidComp.uuid] = handle;
            }

            SceneSerializer::DeserializeEntityComponents(*m_Scene, handle, rec.entityData);

            if (!m_Scene->HasComponent<TagComponent>(handle))
                m_Scene->AddComponent<TagComponent>(handle, "Entity");

            // Restore parent
            entt::entity parentHandle = m_Scene->GetRootEntity();
            if (rec.parentUUID != 0)
            {
                auto it = m_Scene->GetUUIDEntityMap().find(rec.parentUUID);
                if (it != m_Scene->GetUUIDEntityMap().end() && m_Scene->GetRegistry().valid(it->second))
                {
                    parentHandle = it->second;
                }
            }
            m_Scene->SetParentKeepLocal(handle, parentHandle);

            restoredHandles.push_back(handle);
        }

        m_Scene->SetDirty(true);

        if (!restoredHandles.empty())
        {
            EditorSelection::Get().Select(restoredHandles.front());
        }
    }

    void Undo() override
    {
        if (!m_Scene) return;

        auto& uuidMap = m_Scene->GetUUIDEntityMap();
        for (const auto& rec : m_Records)
        {
            auto it = uuidMap.find(rec.uuid);
            if (it != uuidMap.end())
            {
                entt::entity handle = it->second;
                if (handle != m_Scene->GetRootEntity() && m_Scene->GetRegistry().valid(handle))
                {
                    EditorSelection::Get().Remove(handle);
                    m_Scene->DestroyEntity(handle);
                }
            }
        }

        m_Scene->SetDirty(true);
    }

    std::string GetName() const override
    {
        if (!m_Records.empty() && m_Records[0].entityData.contains("Tag"))
        {
            return "Create Entity (" + m_Records[0].entityData["Tag"].value("tag", "Entity") + ")";
        }
        return "Create Entity";
    }

private:
    void CollectSubtree(entt::entity entity)
    {
        if (entity == entt::null || !m_Scene->GetRegistry().valid(entity) || entity == m_Scene->GetRootEntity())
            return;

        EntitySnapshotRecord rec;
        rec.entityData = SceneSerializer::SerializeEntity(*m_Scene, entity, false);
        if (m_Scene->HasComponent<UUIDComponent>(entity))
            rec.uuid = m_Scene->GetComponent<UUIDComponent>(entity).uuid;

        entt::entity parentHandle = m_Scene->GetParent(entity);
        if (parentHandle != entt::null && m_Scene->GetRegistry().valid(parentHandle) && m_Scene->HasComponent<UUIDComponent>(parentHandle))
        {
            rec.parentUUID = m_Scene->GetComponent<UUIDComponent>(parentHandle).uuid;
        }

        m_Records.push_back(rec);

        if (m_Scene->HasComponent<SceneTreeComponent>(entity))
        {
            auto childrenCopy = m_Scene->GetComponent<SceneTreeComponent>(entity).children;
            for (auto child : childrenCopy)
            {
                CollectSubtree(child);
            }
        }
    }

    Scene* m_Scene = nullptr;
    std::vector<EntitySnapshotRecord> m_Records;
    bool m_FirstExecute = false;
};

class DeleteEntityCommand : public Command
{
public:
    DeleteEntityCommand(Scene* scene, const std::vector<entt::entity>& entitiesToDelete)
        : m_Scene(scene)
    {
        if (!m_Scene) return;

        auto& reg = m_Scene->GetRegistry();
        auto root = m_Scene->GetRootEntity();

        std::vector<entt::entity> rootsToDelete;
        for (auto e : entitiesToDelete)
        {
            if (e == entt::null || !reg.valid(e) || e == root)
                continue;

            bool isChildOfOther = false;
            for (auto other : entitiesToDelete)
            {
                if (other != e && other != entt::null && reg.valid(other) && IsDescendantOf(e, other))
                {
                    isChildOfOther = true;
                    break;
                }
            }

            if (!isChildOfOther)
                rootsToDelete.push_back(e);
        }

        for (auto rootE : rootsToDelete)
        {
            CollectSubtree(rootE);
        }
    }

    void Execute() override
    {
        if (!m_Scene) return;

        auto& uuidMap = m_Scene->GetUUIDEntityMap();
        for (const auto& rec : m_Records)
        {
            auto it = uuidMap.find(rec.uuid);
            if (it != uuidMap.end())
            {
                entt::entity handle = it->second;
                if (handle != m_Scene->GetRootEntity() && m_Scene->GetRegistry().valid(handle))
                {
                    EditorSelection::Get().Remove(handle);
                    m_Scene->DestroyEntity(handle);
                }
            }
        }

        m_Scene->SetDirty(true);
    }

    void Undo() override
    {
        if (!m_Scene) return;

        std::vector<entt::entity> restoredHandles;

        for (const auto& rec : m_Records)
        {
            Entity e = m_Scene->CreateEntityRaw();
            entt::entity handle = e.GetHandle();

            if (rec.entityData.contains("UUID"))
            {
                auto& uuidComp = m_Scene->GetComponent<UUIDComponent>(handle);
                uuidComp.Deserialize(rec.entityData["UUID"]);
                m_Scene->GetUUIDEntityMap()[uuidComp.uuid] = handle;
            }

            SceneSerializer::DeserializeEntityComponents(*m_Scene, handle, rec.entityData);

            if (!m_Scene->HasComponent<TagComponent>(handle))
                m_Scene->AddComponent<TagComponent>(handle, "Entity");

            // Restore parent
            entt::entity parentHandle = m_Scene->GetRootEntity();
            if (rec.parentUUID != 0)
            {
                auto it = m_Scene->GetUUIDEntityMap().find(rec.parentUUID);
                if (it != m_Scene->GetUUIDEntityMap().end() && m_Scene->GetRegistry().valid(it->second))
                {
                    parentHandle = it->second;
                }
            }
            m_Scene->SetParentKeepLocal(handle, parentHandle);

            restoredHandles.push_back(handle);
        }

        m_Scene->SetDirty(true);

        if (!restoredHandles.empty())
        {
            EditorSelection::Get().SetSelection(restoredHandles, restoredHandles.front());
        }
    }

    std::string GetName() const override
    {
        return m_Records.size() > 1 ? "Delete Multiple Entities" : "Delete Entity";
    }

private:
    bool IsDescendantOf(entt::entity child, entt::entity parent) const
    {
        if (child == entt::null || parent == entt::null) return false;
        entt::entity cur = m_Scene->GetParent(child);
        while (cur != entt::null && cur != m_Scene->GetRootEntity())
        {
            if (cur == parent) return true;
            cur = m_Scene->GetParent(cur);
        }
        return false;
    }

    void CollectSubtree(entt::entity entity)
    {
        if (entity == entt::null || !m_Scene->GetRegistry().valid(entity) || entity == m_Scene->GetRootEntity())
            return;

        EntitySnapshotRecord rec;
        rec.entityData = SceneSerializer::SerializeEntity(*m_Scene, entity, false);
        if (m_Scene->HasComponent<UUIDComponent>(entity))
            rec.uuid = m_Scene->GetComponent<UUIDComponent>(entity).uuid;

        entt::entity parentHandle = m_Scene->GetParent(entity);
        if (parentHandle != entt::null && m_Scene->GetRegistry().valid(parentHandle) && m_Scene->HasComponent<UUIDComponent>(parentHandle))
        {
            rec.parentUUID = m_Scene->GetComponent<UUIDComponent>(parentHandle).uuid;
        }

        m_Records.push_back(rec);

        if (m_Scene->HasComponent<SceneTreeComponent>(entity))
        {
            auto childrenCopy = m_Scene->GetComponent<SceneTreeComponent>(entity).children;
            for (auto child : childrenCopy)
            {
                CollectSubtree(child);
            }
        }
    }

    Scene* m_Scene = nullptr;
    std::vector<EntitySnapshotRecord> m_Records;
};

class DuplicateEntityCommand : public Command
{
public:
    DuplicateEntityCommand(Scene* scene, const std::vector<entt::entity>& duplicatedEntities)
        : m_Scene(scene), m_FirstExecute(true)
    {
        if (!m_Scene) return;
        for (auto e : duplicatedEntities)
        {
            CollectSubtree(e);
        }
    }

    void Execute() override
    {
        if (!m_Scene) return;

        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            return;
        }

        std::vector<entt::entity> restoredHandles;
        for (const auto& rec : m_Records)
        {
            Entity e = m_Scene->CreateEntityRaw();
            entt::entity handle = e.GetHandle();

            if (rec.entityData.contains("UUID"))
            {
                auto& uuidComp = m_Scene->GetComponent<UUIDComponent>(handle);
                uuidComp.Deserialize(rec.entityData["UUID"]);
                m_Scene->GetUUIDEntityMap()[uuidComp.uuid] = handle;
            }

            SceneSerializer::DeserializeEntityComponents(*m_Scene, handle, rec.entityData);

            if (!m_Scene->HasComponent<TagComponent>(handle))
                m_Scene->AddComponent<TagComponent>(handle, "Entity");

            // Restore parent
            entt::entity parentHandle = m_Scene->GetRootEntity();
            if (rec.parentUUID != 0)
            {
                auto it = m_Scene->GetUUIDEntityMap().find(rec.parentUUID);
                if (it != m_Scene->GetUUIDEntityMap().end() && m_Scene->GetRegistry().valid(it->second))
                {
                    parentHandle = it->second;
                }
            }
            m_Scene->SetParentKeepLocal(handle, parentHandle);

            restoredHandles.push_back(handle);
        }

        m_Scene->SetDirty(true);

        if (!restoredHandles.empty())
        {
            EditorSelection::Get().SetSelection(restoredHandles, restoredHandles.front());
        }
    }

    void Undo() override
    {
        if (!m_Scene) return;

        auto& uuidMap = m_Scene->GetUUIDEntityMap();
        for (const auto& rec : m_Records)
        {
            auto it = uuidMap.find(rec.uuid);
            if (it != uuidMap.end())
            {
                entt::entity handle = it->second;
                if (handle != m_Scene->GetRootEntity() && m_Scene->GetRegistry().valid(handle))
                {
                    EditorSelection::Get().Remove(handle);
                    m_Scene->DestroyEntity(handle);
                }
            }
        }

        m_Scene->SetDirty(true);
    }

    std::string GetName() const override
    {
        return m_Records.size() > 1 ? "Duplicate Multiple Entities" : "Duplicate Entity";
    }

private:
    void CollectSubtree(entt::entity entity)
    {
        if (entity == entt::null || !m_Scene->GetRegistry().valid(entity) || entity == m_Scene->GetRootEntity())
            return;

        EntitySnapshotRecord rec;
        rec.entityData = SceneSerializer::SerializeEntity(*m_Scene, entity, false);
        if (m_Scene->HasComponent<UUIDComponent>(entity))
            rec.uuid = m_Scene->GetComponent<UUIDComponent>(entity).uuid;

        entt::entity parentHandle = m_Scene->GetParent(entity);
        if (parentHandle != entt::null && m_Scene->GetRegistry().valid(parentHandle) && m_Scene->HasComponent<UUIDComponent>(parentHandle))
        {
            rec.parentUUID = m_Scene->GetComponent<UUIDComponent>(parentHandle).uuid;
        }

        m_Records.push_back(rec);

        if (m_Scene->HasComponent<SceneTreeComponent>(entity))
        {
            auto childrenCopy = m_Scene->GetComponent<SceneTreeComponent>(entity).children;
            for (auto child : childrenCopy)
            {
                CollectSubtree(child);
            }
        }
    }

    Scene* m_Scene = nullptr;
    std::vector<EntitySnapshotRecord> m_Records;
    bool m_FirstExecute = false;
};

class RenameEntityCommand : public Command
{
public:
    RenameEntityCommand(Scene* scene, entt::entity entity, const std::string& oldName, const std::string& newName)
        : m_Scene(scene), m_Entity(entity), m_OldName(oldName), m_NewName(newName), m_FirstExecute(true)
    {
        if (m_Scene && entity != entt::null && m_Scene->GetRegistry().valid(entity) && m_Scene->HasComponent<UUIDComponent>(entity))
            m_UUID = m_Scene->GetComponent<UUIDComponent>(entity).uuid;
    }

    void Execute() override
    {
        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            return;
        }
        ApplyName(m_NewName);
    }

    void Undo() override
    {
        ApplyName(m_OldName);
    }

    std::string GetName() const override
    {
        return "Rename Entity";
    }

private:
    void ApplyName(const std::string& name)
    {
        if (!m_Scene) return;
        entt::entity target = m_Entity;
        auto& reg = m_Scene->GetRegistry();
        if (target == entt::null || !reg.valid(target) ||
            (m_UUID != 0 && reg.any_of<UUIDComponent>(target) && reg.get<UUIDComponent>(target).uuid != m_UUID))
        {
            auto it = m_Scene->GetUUIDEntityMap().find(m_UUID);
            if (it != m_Scene->GetUUIDEntityMap().end())
                target = it->second;
            else
                target = entt::null;
        }

        if (target != entt::null && reg.valid(target) && reg.any_of<TagComponent>(target))
        {
            reg.get<TagComponent>(target).tag = name;
            m_Scene->SetDirty(true);
        }
    }

    Scene* m_Scene = nullptr;
    entt::entity m_Entity = entt::null;
    EntityUUID m_UUID = 0;
    std::string m_OldName;
    std::string m_NewName;
    bool m_FirstExecute = false;
};

}
