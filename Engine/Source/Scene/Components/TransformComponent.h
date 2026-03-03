#pragma once
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Math/RvelaMath.h"
#include "json.hpp"

namespace rv {

using json = nlohmann::json;

struct alignas(16) TransformComponent
{
public:
    TransformComponent() = default;
    TransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
        : m_LocalPosition(pos), m_LocalEuler(rot), m_LocalScale(scl)
    {
        m_LocalRotation = math::EulerToQuat(m_LocalEuler);
        m_Dirty = true;
    }

    void SetPosition(const glm::vec3& pos) { m_LocalPosition = pos; m_Dirty = true; }
    void SetRotation(const glm::quat& rot) {
        m_LocalRotation = rot;
        m_LocalEuler = math::QuatToEuler(rot);
        m_Dirty = true;
    }
    void SetEulerRotation(const glm::vec3& euler) {
        m_LocalEuler = euler;
        m_LocalRotation = math::EulerToQuat(euler);
        m_Dirty = true;
    }
    void SetScale(const glm::vec3& scl) {
        if (m_LockScaleRatio) {
            float ratio = (m_LocalScale.x != 0.0f) ? (scl.x / m_LocalScale.x) : 1.0f;
            m_LocalScale *= ratio;
        }
        else {
            m_LocalScale = scl;
        }
        m_Dirty = true;
    }

    const glm::vec3& GetPosition() const { return m_LocalPosition; }
    const glm::quat& GetRotation() const { return m_LocalRotation; }
    const glm::vec3& GetEulerRotation() const { return m_LocalEuler; }
    const glm::vec3& GetScale() const { return m_LocalScale; }

    glm::vec3 GetWorldPosition() const { return glm::vec3(m_WorldMatrix[3]); }
    glm::quat GetWorldRotation() const {
        glm::vec3 s; glm::quat q; glm::vec3 t; glm::vec3 sk; glm::vec4 p;
        glm::decompose(m_WorldMatrix, s, q, t, sk, p);
        return q;
    }
    glm::vec3 GetWorldScale() const {
        glm::vec3 s; glm::quat q; glm::vec3 t; glm::vec3 sk; glm::vec4 p;
        glm::decompose(m_WorldMatrix, s, q, t, sk, p);
        return s;
    }

    void SetLockScaleRatio(bool lock) { m_LockScaleRatio = lock; }
    bool IsScaleRatioLocked() const { return m_LockScaleRatio; }

    bool IsDirty() const { return m_Dirty; }
    void SetDirty(bool dirty = true) { m_Dirty = dirty; }

    void SetWorldMatrix(const glm::mat4& matrix) {
        m_WorldMatrix = matrix;
        m_Dirty = false;
    }
    const glm::mat4& GetWorldMatrix() const { return m_WorldMatrix; }

    inline glm::mat4 GetLocalMatrix() const noexcept
    {
        return glm::translate(glm::mat4(1.0f), m_LocalPosition)
            * glm::mat4_cast(m_LocalRotation)
            * glm::scale(glm::mat4(1.0f), m_LocalScale);
    }

    inline void Translate(const glm::vec3& delta) noexcept {
        m_LocalPosition += delta;
        m_Dirty = true;
    }

    void Rotate(float angleDeg, const glm::vec3& axis) noexcept {
        glm::quat rotationDelta = glm::angleAxis(glm::radians(angleDeg), axis);
        m_LocalRotation = rotationDelta * m_LocalRotation;
        m_LocalEuler = math::QuatToEuler(m_LocalRotation);
        m_Dirty = true;
    }

    void Move(const glm::vec3& direction) noexcept {
        m_LocalPosition += direction;
        m_Dirty = true;
    }

    glm::vec3 GetForward() const { return glm::normalize(glm::vec3(m_WorldMatrix[2]) * -1.0f); }
    glm::vec3 GetUp()      const { return glm::normalize(glm::vec3(m_WorldMatrix[1])); }
    glm::vec3 GetRight()   const { return glm::normalize(glm::vec3(m_WorldMatrix[0])); }

    void LookAt(const glm::vec3& target, const glm::vec3& up) noexcept;

    json Serialize() const noexcept;
    void Deserialize(const json& j) noexcept;

private:
    glm::vec3 m_LocalPosition = glm::vec3(0.0f);
    glm::quat m_LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 m_LocalScale = glm::vec3(1.0f);
    glm::vec3 m_LocalEuler = glm::vec3(0.0f);

    glm::mat4 m_WorldMatrix = glm::mat4(1.0f);

    glm::vec3 m_WorldPositionCache{ 0.0f };
    glm::quat m_WorldRotationCache{ 1.0f, 0.0f, 0.0f, 0.0f };

    bool m_Dirty = true;
    bool m_LockScaleRatio = false;
    glm::vec3 m_ScaleRatio = glm::vec3(1.0f);
};

}