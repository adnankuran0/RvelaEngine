#pragma once
#include "Command.h"
#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Scene/Components/UIComponents.h"
#include "Scene/EntityUUID.h"
#include "UI/UILayout.h"
#include <glm/glm.hpp>
#include <vector>

namespace rv {

struct EntityRectTransformRecord
{
    EntityUUID uuid = 0;
    entt::entity entity = entt::null;
    glm::vec2 position{ 0.0f };
    glm::vec2 size{ 100.0f };
    float rotation = 0.0f;
    glm::vec2 scale{ 1.0f };
};

class RectTransformCommand : public Command
{
public:
    RectTransformCommand(Scene* scene, const std::vector<EntityRectTransformRecord>& before, const std::vector<EntityRectTransformRecord>& after, const std::string& customName = "")
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
        return m_Before.size() > 1 ? "Transform Multiple UI Entities" : "Transform UI Entity";
    }

private:
    void ApplyTransforms(const std::vector<EntityRectTransformRecord>& records)
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

            if (e != entt::null && reg.valid(e) && reg.any_of<RectTransformComponent>(e))
            {
                auto& rtc = reg.get<RectTransformComponent>(e);
                rtc.position = rec.position;
                rtc.size = rec.size;
                rtc.rotation = rec.rotation;
                rtc.scale = rec.scale;
            }
        }

        m_Scene->SetDirty(true);
        UILayout::UpdateCanvasLayouts(m_Scene, 1920.0f, 1080.0f);
    }

    Scene* m_Scene = nullptr;
    std::vector<EntityRectTransformRecord> m_Before;
    std::vector<EntityRectTransformRecord> m_After;
    bool m_FirstExecute = false;
    std::string m_CustomName;
};

}
