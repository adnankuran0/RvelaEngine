#pragma once
#include <entt/entt.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/Components/SpotLightComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/ParticleEmitterComponent.h"
#include "Scene/Components/AudioEmitterComponent.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/ColliderComponents.h"

namespace rv::Animation {

    class PropertyBindingRegistry {
    public:
        static PropertyBindingRegistry& Get() {
            static PropertyBindingRegistry instance;
            return instance;
        }

        void Init() {
            if (m_Initialized) return;

            RegisterVec3("TransformComponent.position",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<TransformComponent>(e); },
                [](entt::registry& reg, entt::entity e, const glm::vec3& val) { reg.get<TransformComponent>(e).SetPosition(val); },
                [](entt::registry& reg, entt::entity e) { return reg.get<TransformComponent>(e).GetPosition(); }
            );

            RegisterQuat("TransformComponent.rotation",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<TransformComponent>(e); },
                [](entt::registry& reg, entt::entity e, const glm::quat& val) { reg.get<TransformComponent>(e).SetRotation(val); },
                [](entt::registry& reg, entt::entity e) { return reg.get<TransformComponent>(e).GetRotation(); }
            );

            RegisterVec3("TransformComponent.scale",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<TransformComponent>(e); },
                [](entt::registry& reg, entt::entity e, const glm::vec3& val) { reg.get<TransformComponent>(e).SetScale(val); },
                [](entt::registry& reg, entt::entity e) { return reg.get<TransformComponent>(e).GetScale(); }
            );

            RegisterVec3("PointLightComponent.color",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<PointLightComponent>(e); },
                [](entt::registry& reg, entt::entity e, const glm::vec3& val) { reg.get<PointLightComponent>(e).color = val; },
                [](entt::registry& reg, entt::entity e) { return reg.get<PointLightComponent>(e).color; }
            );

            RegisterFloat("PointLightComponent.intensity",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<PointLightComponent>(e); },
                [](entt::registry& reg, entt::entity e, float val) { reg.get<PointLightComponent>(e).intensity = val; },
                [](entt::registry& reg, entt::entity e) { return reg.get<PointLightComponent>(e).intensity; }
            );

            RegisterFloat("PointLightComponent.radius",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<PointLightComponent>(e); },
                [](entt::registry& reg, entt::entity e, float val) { reg.get<PointLightComponent>(e).radius = val; },
                [](entt::registry& reg, entt::entity e) { return reg.get<PointLightComponent>(e).radius; }
            );

            RegisterBool("PointLightComponent.castShadows",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<PointLightComponent>(e); },
                [](entt::registry& reg, entt::entity e, bool val) { reg.get<PointLightComponent>(e).castShadows = val; },
                [](entt::registry& reg, entt::entity e) { return reg.get<PointLightComponent>(e).castShadows; }
            );

            RegisterFloat("CameraComponent.FOV",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<CameraComponent>(e); },
                [](entt::registry& reg, entt::entity e, float val) { reg.get<CameraComponent>(e).SetFOV(val); },
                [](entt::registry& reg, entt::entity e) { return reg.get<CameraComponent>(e).GetFOV(); }
            );

            RegisterBool("CameraComponent.isActive",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<CameraComponent>(e); },
                [](entt::registry& reg, entt::entity e, bool val) { reg.get<CameraComponent>(e).isActive = val; },
                [](entt::registry& reg, entt::entity e) { return reg.get<CameraComponent>(e).isActive; }
            );

            RegisterVec4("MaterialComponent.albedoColor",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<MaterialComponent>(e); },
                [](entt::registry& reg, entt::entity e, const glm::vec4& val) { reg.get<MaterialComponent>(e).SetAlbedoColor(val); },
                [](entt::registry& reg, entt::entity e) { return reg.get<MaterialComponent>(e).GetAlbedoColor(); }
            );

            RegisterVec3("MaterialComponent.emissiveColor",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<MaterialComponent>(e); },
                [](entt::registry& reg, entt::entity e, const glm::vec3& val) { reg.get<MaterialComponent>(e).SetEmissiveColor(val); },
                [](entt::registry& reg, entt::entity e) { return reg.get<MaterialComponent>(e).GetEmissiveColor(); }
            );

            RegisterFloat("MaterialComponent.metallic",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<MaterialComponent>(e); },
                [](entt::registry& reg, entt::entity e, float val) { reg.get<MaterialComponent>(e).SetMetallic(val); },
                [](entt::registry& reg, entt::entity e) { return reg.get<MaterialComponent>(e).GetMetallic(); }
            );

            RegisterFloat("MaterialComponent.roughness",
                [](entt::registry& reg, entt::entity e) { return reg.any_of<MaterialComponent>(e); },
                [](entt::registry& reg, entt::entity e, float val) { reg.get<MaterialComponent>(e).SetRoughness(val); },
                [](entt::registry& reg, entt::entity e) { return reg.get<MaterialComponent>(e).GetRoughness(); }
            );

            m_Initialized = true;
        }

        template<typename T>
        T GetCurrentValue(entt::registry& reg, entt::entity e, const std::string& path, const T& fallback = T{}) {
            if constexpr (std::is_same_v<T, bool>) {
                auto it = m_BoolBindings.find(path);
                if (it != m_BoolBindings.end() && it->second.isValid(reg, e)) return it->second.getter(reg, e);
            }
            else if constexpr (std::is_same_v<T, float>) {
                auto it = m_FloatBindings.find(path);
                if (it != m_FloatBindings.end() && it->second.isValid(reg, e)) return it->second.getter(reg, e);
            }
            else if constexpr (std::is_same_v<T, glm::vec2>) {
                auto it = m_Vec2Bindings.find(path);
                if (it != m_Vec2Bindings.end() && it->second.isValid(reg, e)) return it->second.getter(reg, e);
            }
            else if constexpr (std::is_same_v<T, glm::vec3>) {
                auto it = m_Vec3Bindings.find(path);
                if (it != m_Vec3Bindings.end() && it->second.isValid(reg, e)) return it->second.getter(reg, e);
            }
            else if constexpr (std::is_same_v<T, glm::vec4>) {
                auto it = m_Vec4Bindings.find(path);
                if (it != m_Vec4Bindings.end() && it->second.isValid(reg, e)) return it->second.getter(reg, e);
            }
            else if constexpr (std::is_same_v<T, glm::quat>) {
                auto it = m_QuatBindings.find(path);
                if (it != m_QuatBindings.end() && it->second.isValid(reg, e)) return it->second.getter(reg, e);
            }
            return fallback;
        }

        void Apply(entt::registry& reg, entt::entity e, const std::string& path, bool val) {
            if (auto it = m_BoolBindings.find(path); it != m_BoolBindings.end() && it->second.isValid(reg, e))
                it->second.setter(reg, e, val);
        }
        void Apply(entt::registry& reg, entt::entity e, const std::string& path, float val) {
            if (auto it = m_FloatBindings.find(path); it != m_FloatBindings.end() && it->second.isValid(reg, e))
                it->second.setter(reg, e, val);
        }
        void Apply(entt::registry& reg, entt::entity e, const std::string& path, const glm::vec3& val) {
            if (auto it = m_Vec3Bindings.find(path); it != m_Vec3Bindings.end() && it->second.isValid(reg, e))
                it->second.setter(reg, e, val);
        }
        void Apply(entt::registry& reg, entt::entity e, const std::string& path, const glm::vec4& val) {
            if (auto it = m_Vec4Bindings.find(path); it != m_Vec4Bindings.end() && it->second.isValid(reg, e))
                it->second.setter(reg, e, val);
        }
        void Apply(entt::registry& reg, entt::entity e, const std::string& path, const glm::quat& val) {
            if (auto it = m_QuatBindings.find(path); it != m_QuatBindings.end() && it->second.isValid(reg, e))
                it->second.setter(reg, e, val);
        }
        void Apply(entt::registry& reg, entt::entity e, const std::string& path, const glm::vec2& val) {
            if (auto it = m_Vec2Bindings.find(path); it != m_Vec2Bindings.end() && it->second.isValid(reg, e))
                it->second.setter(reg, e, val);
        }

        const auto& GetBoolBindings() const { return m_BoolBindings; }
        const auto& GetFloatBindings() const { return m_FloatBindings; }
        const auto& GetVec3Bindings() const { return m_Vec3Bindings; }
        const auto& GetVec4Bindings() const { return m_Vec4Bindings; }
        const auto& GetQuatBindings() const { return m_QuatBindings; }
    private:
        bool m_Initialized = false;

        template<typename T>
        struct Binding {
            std::function<bool(entt::registry&, entt::entity)> isValid;
            std::function<void(entt::registry&, entt::entity, const T&)> setter;
            std::function<T(entt::registry&, entt::entity)> getter;
        };

        std::unordered_map<std::string, Binding<bool>> m_BoolBindings;
        std::unordered_map<std::string, Binding<float>> m_FloatBindings;
        std::unordered_map<std::string, Binding<glm::vec2>> m_Vec2Bindings;
        std::unordered_map<std::string, Binding<glm::vec3>> m_Vec3Bindings;
        std::unordered_map<std::string, Binding<glm::vec4>> m_Vec4Bindings;
        std::unordered_map<std::string, Binding<glm::quat>> m_QuatBindings;

        void RegisterBool(const std::string& p, auto valid, auto set, auto get) { m_BoolBindings[p] = { valid, set, get }; }
        void RegisterFloat(const std::string& p, auto valid, auto set, auto get) { m_FloatBindings[p] = { valid, set, get }; }
        void RegisterVec2(const std::string& p, auto valid, auto set, auto get) { m_Vec2Bindings[p] = { valid, set, get }; }
        void RegisterVec3(const std::string& p, auto valid, auto set, auto get) { m_Vec3Bindings[p] = { valid, set, get }; }
        void RegisterVec4(const std::string& p, auto valid, auto set, auto get) { m_Vec4Bindings[p] = { valid, set, get }; }
        void RegisterQuat(const std::string& p, auto valid, auto set, auto get) { m_QuatBindings[p] = { valid, set, get }; }
    };

}