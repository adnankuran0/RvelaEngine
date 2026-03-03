#include "rvelapch.h"
#include "TransformComponent.h"
#include <glm/gtx/quaternion.hpp>

using namespace rv;

void TransformComponent::LookAt(const glm::vec3& target, const glm::vec3& up) noexcept
{
    glm::vec3 worldPos = GetWorldPosition();
    glm::vec3 direction = target - worldPos;

    if (glm::length2(direction) > 0.0001f)
    {
        direction = glm::normalize(direction);
        m_LocalRotation = glm::quatLookAt(direction, up);
        m_LocalEuler = math::QuatToEuler(m_LocalRotation);
        m_Dirty = true;
    }
}

json TransformComponent::Serialize() const noexcept
{
    json j;
    j["position"] = { m_LocalPosition.x, m_LocalPosition.y, m_LocalPosition.z };
    j["rotation"] = { m_LocalEuler.x, m_LocalEuler.y, m_LocalEuler.z };
    j["scale"] = { m_LocalScale.x, m_LocalScale.y, m_LocalScale.z };
    j["lockScaleRatio"] = m_LockScaleRatio;

    return j;
}

void TransformComponent::Deserialize(const json& j) noexcept
{
    if (j.contains("position")) {
        auto& p = j.at("position");
        m_LocalPosition = glm::vec3(p[0], p[1], p[2]);
    }

    if (j.contains("rotation")) {
        auto& r = j.at("rotation");
        SetEulerRotation(glm::vec3(r[0], r[1], r[2]));
    }

    if (j.contains("scale")) {
        auto& s = j.at("scale");
        m_LocalScale = glm::vec3(s[0], s[1], s[2]);
    }

    if (j.contains("lockScaleRatio"))
        m_LockScaleRatio = j.at("lockScaleRatio").get<bool>();

    if (j.contains("scaleRatio")) {
        auto& sr = j.at("scaleRatio");
    }

    m_Dirty = true;
}