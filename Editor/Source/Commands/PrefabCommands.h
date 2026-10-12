#pragma once
#include "Command.h"
#include "Scene/Scene.h"
#include "Scene/EntityUUID.h"
#include "Scene/SceneSerializer.h"
#include "Scene/Components/PrefabComponent.h"
#include "AssetImporters/PrefabImporter.h"
#include "Asset/AssetManager.h"
#include "EditorSelection.h"
#include "EntityLifecycleCommands.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace rv {

class UnpackPrefabCommand : public Command
{
public:
    UnpackPrefabCommand(Scene* scene, entt::entity entity)
        : m_Scene(scene), m_Entity(entity), m_FirstExecute(true)
    {
        if (m_Scene && entity != entt::null && m_Scene->GetRegistry().valid(entity))
        {
            if (m_Scene->HasComponent<UUIDComponent>(entity))
                m_UUID = m_Scene->GetComponent<UUIDComponent>(entity).uuid;

            if (m_Scene->HasComponent<PrefabComponent>(entity))
            {
                m_SavedPrefabComp = m_Scene->GetComponent<PrefabComponent>(entity);
                m_HasSaved = true;
            }
        }
    }

    void Execute() override
    {
        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            return;
        }

        entt::entity target = GetTarget();
        if (target != entt::null && m_Scene->HasComponent<PrefabComponent>(target))
        {
            m_Scene->RemoveComponent<PrefabComponent>(target);
            m_Scene->SetDirty(true);
        }
    }

    void Undo() override
    {
        entt::entity target = GetTarget();
        if (target != entt::null && m_HasSaved)
        {
            m_Scene->AddComponent<PrefabComponent>(target, m_SavedPrefabComp.GetPrefabID(), m_SavedPrefabComp.GetOverrides());
            m_Scene->SetDirty(true);
        }
    }

    std::string GetName() const override
    {
        return "Unpack Prefab";
    }

private:
    entt::entity GetTarget() const
    {
        if (!m_Scene) return entt::null;
        auto& reg = m_Scene->GetRegistry();
        if (m_Entity != entt::null && reg.valid(m_Entity))
        {
            if (m_UUID == 0 || (reg.any_of<UUIDComponent>(m_Entity) && reg.get<UUIDComponent>(m_Entity).uuid == m_UUID))
                return m_Entity;
        }
        if (m_UUID != 0)
        {
            auto it = m_Scene->GetUUIDEntityMap().find(m_UUID);
            if (it != m_Scene->GetUUIDEntityMap().end() && reg.valid(it->second))
                return it->second;
        }
        return entt::null;
    }

    Scene* m_Scene = nullptr;
    entt::entity m_Entity = entt::null;
    EntityUUID m_UUID = 0;
    PrefabComponent m_SavedPrefabComp;
    bool m_HasSaved = false;
    bool m_FirstExecute = false;
};

class RevertPrefabCommand : public Command
{
public:
    RevertPrefabCommand(Scene* scene, entt::entity oldRoot)
        : m_Scene(scene), m_FirstExecute(true)
    {
        if (!m_Scene || oldRoot == entt::null) return;
        CollectSubtree(oldRoot, m_BeforeRecords);
    }

    void CaptureAfter(entt::entity newRoot)
    {
        if (!m_Scene || newRoot == entt::null) return;
        CollectSubtree(newRoot, m_AfterRecords);
    }

    void Execute() override
    {
        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            return;
        }

        DestroyRecords(m_BeforeRecords);
        RestoreRecords(m_AfterRecords);
    }

    void Undo() override
    {
        DestroyRecords(m_AfterRecords);
        RestoreRecords(m_BeforeRecords);
    }

    std::string GetName() const override
    {
        return "Revert to Prefab";
    }

private:
    void DestroyRecords(const std::vector<EntitySnapshotRecord>& records)
    {
        if (!m_Scene) return;
        auto& uuidMap = m_Scene->GetUUIDEntityMap();
        for (const auto& rec : records)
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

    void RestoreRecords(const std::vector<EntitySnapshotRecord>& records)
    {
        if (!m_Scene) return;
        std::vector<entt::entity> restored;
        for (const auto& rec : records)
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
            restored.push_back(handle);
        }
        m_Scene->SetDirty(true);
        if (!restored.empty())
        {
            EditorSelection::Get().Select(restored.front());
        }
    }

    void CollectSubtree(entt::entity entity, std::vector<EntitySnapshotRecord>& outRecords)
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

        outRecords.push_back(rec);

        if (m_Scene->HasComponent<SceneTreeComponent>(entity))
        {
            auto childrenCopy = m_Scene->GetComponent<SceneTreeComponent>(entity).children;
            for (auto child : childrenCopy)
            {
                CollectSubtree(child, outRecords);
            }
        }
    }

    Scene* m_Scene = nullptr;
    std::vector<EntitySnapshotRecord> m_BeforeRecords;
    std::vector<EntitySnapshotRecord> m_AfterRecords;
    bool m_FirstExecute = false;
};

class ApplyPrefabCommand : public Command
{
public:
    ApplyPrefabCommand(Scene* scene, entt::entity rootEntity)
        : m_Scene(scene), m_RootEntity(rootEntity), m_FirstExecute(true)
    {
        if (!m_Scene || rootEntity == entt::null || !m_Scene->HasComponent<PrefabComponent>(rootEntity)) return;

        m_PrefabUUID = m_Scene->GetComponent<PrefabComponent>(rootEntity).GetPrefabID();
        if (!m_PrefabUUID.IsValid()) return;

        AssetRegistry& reg = AssetManager::Get().GetRegistry();
        m_PrefabPath = reg.GetPath(m_PrefabUUID);

        if (std::filesystem::exists(m_PrefabPath))
        {
            std::ifstream file(m_PrefabPath);
            if (file)
            {
                std::stringstream buffer;
                buffer << file.rdbuf();
                m_OldFileContent = buffer.str();
            }
        }
    }

    void CaptureNewContent()
    {
        if (std::filesystem::exists(m_PrefabPath))
        {
            std::ifstream file(m_PrefabPath);
            if (file)
            {
                std::stringstream buffer;
                buffer << file.rdbuf();
                m_NewFileContent = buffer.str();
            }
        }
    }

    void Execute() override
    {
        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            CaptureNewContent();
            return;
        }

        ApplyFileContent(m_NewFileContent);
    }

    void Undo() override
    {
        ApplyFileContent(m_OldFileContent);
    }

    std::string GetName() const override
    {
        return "Apply to Prefab";
    }

private:
    void ApplyFileContent(const std::string& content)
    {
        if (m_PrefabPath.empty() || content.empty() || !m_Scene) return;

        {
            std::ofstream file(m_PrefabPath);
            if (file)
            {
                file << content;
            }
        }

        AssetManager::Get().Unload(m_PrefabUUID);

        auto view = m_Scene->GetRegistry().view<PrefabComponent>();
        std::vector<entt::entity> instances;
        for (auto e : view)
        {
            if (view.get<PrefabComponent>(e).GetPrefabID() == m_PrefabUUID)
            {
                instances.push_back(e);
            }
        }

        for (auto inst : instances)
        {
            if (m_Scene->GetRegistry().valid(inst))
            {
                PrefabImporter::RevertPrefab(*m_Scene, inst, true);
            }
        }

        EditorSelection::Get().Validate(m_Scene->GetRegistry());
        m_Scene->SetDirty(true);
    }

    Scene* m_Scene = nullptr;
    entt::entity m_RootEntity = entt::null;
    AssetUUID m_PrefabUUID = AssetUUID::Invalid();
    std::filesystem::path m_PrefabPath;
    std::string m_OldFileContent;
    std::string m_NewFileContent;
    bool m_FirstExecute = false;
};

}
