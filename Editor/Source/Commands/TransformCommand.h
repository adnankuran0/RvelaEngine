#pragma once
#include "Command.h"
#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Scene/EntityUUID.h"
#include <glm/glm.hpp>
#include <vector>

namespace rv {

struct EntityTransformRecord
{
    EntityUUID uuid = 0;
    entt::entity entity = entt::null;
    glm::vec3 position{ 0.0f };
    glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f };
};

class TransformCommand : public Command
{
public:
    TransformCommand(Scene* scene, const std::vector<EntityTransformRecord>& before, const std::vector<EntityTransformRecord>& after, const std::string& customName = "")
        : m_Scene(scene), m_Before(before), m_After(after), m_FirstExecute(true), m_CustomName(customName)
    {
    }

    void SetScene(Scene* scene) override
    {
        m_Scene = scene;
    }

    void Execute() override
    {
        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            return;
        }

        ApplyTransforms(m_After);
    }

    void Undo() override
    {
        ApplyTransforms(m_Before);
    }

    std::string GetName() const override
    {
        if (!m_CustomName.empty())
            return m_CustomName;
        return m_Before.size() > 1 ? "Transform Multiple Entities" : "Transform Entity";
    }

private:
    void ApplyTransforms(const std::vector<EntityTransformRecord>& records)
    {
        if (!m_Scene && Engine::Get())
            m_Scene = &Engine::Get()->GetActiveScene();

        if (!m_Scene) return;

        auto& uuidMap = m_Scene->GetUUIDEntityMap();
        auto& reg = m_Scene->GetRegistry();

        for (const auto& rec : records)
        {
            entt::entity e = rec.entity;
            if (e == entt::null || !reg.valid(e) ||
                (rec.uuid != 0 && reg.any_of<UUIDComponent>(e) && reg.get<UUIDComponent>(e).uuid != rec.uuid))
            {
                auto it = uuidMap.find(rec.uuid);
                if (it != uuidMap.end())
                    e = it->second;
                else
                    e = entt::null;
            }

            if (e != entt::null && reg.valid(e) && reg.any_of<TransformComponent>(e))
            {
                auto& tc = reg.get<TransformComponent>(e);
                tc.SetPosition(rec.position);
                tc.SetRotation(rec.rotation);
                tc.SetScale(rec.scale);
                tc.SetDirty();
            }
        }

        m_Scene->SetDirty(true);
    }

    Scene* m_Scene = nullptr;
    std::vector<EntityTransformRecord> m_Before;
    std::vector<EntityTransformRecord> m_After;
    bool m_FirstExecute = false;
    std::string m_CustomName;
};

}
