#pragma once
#include "Command.h"
#include "Scene/Scene.h"
#include "Scene/EntityUUID.h"
#include "Scene/SceneSerializer.h"
#include "Scene/Components.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/SkeletalMeshRendererComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/SkeletalMeshComponent.h"
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>

namespace rv {

template<typename Comp, typename Enable = void>
struct ComponentStorage
{
    bool hasValue = false;
    Comp value{};

    void Capture(const Comp& src)
    {
        value = src;
        hasValue = true;
    }

    void Restore(entt::registry& reg, entt::entity target)
    {
        if (hasValue)
            reg.emplace_or_replace<Comp>(target, value);
        else
            reg.emplace_or_replace<Comp>(target);
    }
};

template<typename Comp>
struct ComponentStorage<Comp, std::enable_if_t<!std::is_copy_constructible_v<Comp>>>
{
    bool hasValue = false;
    bool castShadow = true;

    void Capture(const Comp& src)
    {
        castShadow = src.IsCastShadow();
        hasValue = true;
    }

    void Restore(entt::registry& reg, entt::entity target)
    {
        if constexpr (std::is_same_v<Comp, MeshRendererComponent>)
        {
            if (reg.any_of<MeshComponent>(target))
            {
                auto& mc = reg.get<MeshComponent>(target);
                auto& r = reg.emplace_or_replace<MeshRendererComponent>(target, mc.GetMesh());
                r.SetCastShadow(castShadow);
            }
            else
            {
                auto& r = reg.emplace_or_replace<MeshRendererComponent>(target);
                r.SetCastShadow(castShadow);
            }
        }
        else if constexpr (std::is_same_v<Comp, SkeletalMeshRendererComponent>)
        {
            if (reg.any_of<SkeletalMeshComponent>(target))
            {
                auto& smc = reg.get<SkeletalMeshComponent>(target);
                auto& r = reg.emplace_or_replace<SkeletalMeshRendererComponent>(target, smc.GetMesh());
                r.SetCastShadow(castShadow);
            }
            else
            {
                auto& r = reg.emplace_or_replace<SkeletalMeshRendererComponent>(target);
                r.SetCastShadow(castShadow);
            }
        }
        else
        {
            reg.emplace_or_replace<Comp>(target);
        }
    }
};

template<typename T>
class ComponentCommand : public Command
{
public:
    enum class Action { Add, Remove };

    ComponentCommand(Scene* scene, entt::entity entity, Action action, const std::string& componentName)
        : m_Scene(scene), m_Entity(entity), m_Action(action), m_ComponentName(componentName), m_FirstExecute(true)
    {
        if (m_Scene && entity != entt::null && m_Scene->GetRegistry().valid(entity))
        {
            if (m_Scene->HasComponent<UUIDComponent>(entity))
                m_UUID = m_Scene->GetComponent<UUIDComponent>(entity).uuid;

            if (m_Scene->HasComponent<T>(entity))
            {
                m_Storage.Capture(m_Scene->GetComponent<T>(entity));
            }
        }
    }

    void SetScene(Scene* scene) override { m_Scene = scene; }

    void Execute() override
    {
        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            if (m_Action == Action::Add && !m_Storage.hasValue)
            {
                entt::entity target = GetTarget();
                if (target != entt::null && m_Scene->HasComponent<T>(target))
                {
                    m_Storage.Capture(m_Scene->GetComponent<T>(target));
                }
            }
            return;
        }

        entt::entity target = GetTarget();
        if (target == entt::null) return;

        if (m_Action == Action::Add)
        {
            m_Storage.Restore(m_Scene->GetRegistry(), target);
        }
        else
        {
            if (m_Scene->HasComponent<T>(target))
                m_Scene->GetRegistry().remove<T>(target);
        }
        m_Scene->SetDirty(true);
    }

    void Undo() override
    {
        entt::entity target = GetTarget();
        if (target == entt::null) return;

        if (m_Action == Action::Add)
        {
            if (m_Scene->HasComponent<T>(target))
                m_Scene->GetRegistry().remove<T>(target);
        }
        else
        {
            m_Storage.Restore(m_Scene->GetRegistry(), target);
        }
        m_Scene->SetDirty(true);
    }

    std::string GetName() const override
    {
        return (m_Action == Action::Add ? "Add " : "Remove ") + m_ComponentName;
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
    Action m_Action;
    std::string m_ComponentName;
    ComponentStorage<T> m_Storage;
    bool m_FirstExecute = false;
};

class EntitySnapshotCommand : public Command
{
public:
    EntitySnapshotCommand(Scene* scene, entt::entity entity, const json& before, const json& after, const std::string& actionName = "Edit Property")
        : m_Scene(scene), m_Entity(entity), m_Before(before), m_After(after), m_ActionName(actionName), m_FirstExecute(true)
    {
        if (m_Scene && entity != entt::null && m_Scene->GetRegistry().valid(entity) && m_Scene->HasComponent<UUIDComponent>(entity))
            m_UUID = m_Scene->GetComponent<UUIDComponent>(entity).uuid;
    }

    void SetScene(Scene* scene) override { m_Scene = scene; }

    void Execute() override
    {
        if (m_FirstExecute)
        {
            m_FirstExecute = false;
            return;
        }
        Apply(m_After);
    }

    void Undo() override
    {
        Apply(m_Before);
    }

    std::string GetName() const override
    {
        return m_ActionName;
    }

private:
    static void ClearOptionalComponents(Scene& scene, entt::entity target)
    {
        auto& reg = scene.GetRegistry();
        if (reg.any_of<TagComponent>(target)) reg.remove<TagComponent>(target);
        if (reg.any_of<MaterialComponent>(target)) reg.remove<MaterialComponent>(target);
        if (reg.any_of<MeshRendererComponent>(target)) reg.remove<MeshRendererComponent>(target);
        if (reg.any_of<MeshComponent>(target)) reg.remove<MeshComponent>(target);
        if (reg.any_of<SkeletalMeshRendererComponent>(target)) reg.remove<SkeletalMeshRendererComponent>(target);
        if (reg.any_of<SkeletalMeshComponent>(target)) reg.remove<SkeletalMeshComponent>(target);
        if (reg.any_of<SkeletonComponent>(target)) reg.remove<SkeletonComponent>(target);
        if (reg.any_of<PointLightComponent>(target)) reg.remove<PointLightComponent>(target);
        if (reg.any_of<DirectionalLightComponent>(target)) reg.remove<DirectionalLightComponent>(target);
        if (reg.any_of<SpotLightComponent>(target)) reg.remove<SpotLightComponent>(target);
        if (reg.any_of<CameraComponent>(target)) reg.remove<CameraComponent>(target);
        if (reg.any_of<ScriptComponent>(target)) reg.remove<ScriptComponent>(target);
        if (reg.any_of<RigidbodyComponent>(target)) reg.remove<RigidbodyComponent>(target);
        if (reg.any_of<CharacterBodyComponent>(target)) reg.remove<CharacterBodyComponent>(target);
        if (reg.any_of<BoxColliderComponent>(target)) reg.remove<BoxColliderComponent>(target);
        if (reg.any_of<SphereColliderComponent>(target)) reg.remove<SphereColliderComponent>(target);
        if (reg.any_of<CapsuleColliderComponent>(target)) reg.remove<CapsuleColliderComponent>(target);
        if (reg.any_of<CylinderColliderComponent>(target)) reg.remove<CylinderColliderComponent>(target);
        if (reg.any_of<MeshColliderComponent>(target)) reg.remove<MeshColliderComponent>(target);
        if (reg.any_of<ConvexHullColliderComponent>(target)) reg.remove<ConvexHullColliderComponent>(target);
        if (reg.any_of<AudioEmitterComponent>(target)) reg.remove<AudioEmitterComponent>(target);
        if (reg.any_of<ParticleEmitterComponent>(target)) reg.remove<ParticleEmitterComponent>(target);
        if (reg.any_of<DecalComponent>(target)) reg.remove<DecalComponent>(target);
        if (reg.any_of<AnimatorComponent>(target)) reg.remove<AnimatorComponent>(target);
        if (reg.any_of<UICanvasComponent>(target)) reg.remove<UICanvasComponent>(target);
        if (reg.any_of<RectTransformComponent>(target)) reg.remove<RectTransformComponent>(target);
        if (reg.any_of<UIImageComponent>(target)) reg.remove<UIImageComponent>(target);
        if (reg.any_of<UITextComponent>(target)) reg.remove<UITextComponent>(target);
        if (reg.any_of<UIButtonComponent>(target)) reg.remove<UIButtonComponent>(target);
        if (reg.any_of<UISliderComponent>(target)) reg.remove<UISliderComponent>(target);
        if (reg.any_of<UIProgressBarComponent>(target)) reg.remove<UIProgressBarComponent>(target);
        if (reg.any_of<UICheckboxComponent>(target)) reg.remove<UICheckboxComponent>(target);
        if (reg.any_of<PrefabComponent>(target)) reg.remove<PrefabComponent>(target);
    }

    void Apply(const json& snapshot)
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

        if (target != entt::null && reg.valid(target))
        {
            ClearOptionalComponents(*m_Scene, target);
            SceneSerializer::DeserializeEntityComponents(*m_Scene, target, snapshot);
            if (!m_Scene->HasComponent<TagComponent>(target))
                m_Scene->AddComponent<TagComponent>(target, "Entity");
            m_Scene->SetDirty(true);
        }
    }

    Scene* m_Scene = nullptr;
    entt::entity m_Entity = entt::null;
    EntityUUID m_UUID = 0;
    json m_Before;
    json m_After;
    std::string m_ActionName;
    bool m_FirstExecute = false;
};

}
