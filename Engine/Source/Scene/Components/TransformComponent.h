#pragma once
#include <cmath>
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
        : m_LocalPosition(pos), m_LocalEuler(rot), m_LocalScale(scl),
          m_WorldPositionCache(pos), m_WorldScaleCache(scl),
          m_Dirty(true)
    {
        m_LocalRotation = math::EulerToQuat(m_LocalEuler);
        m_WorldRotationCache = m_LocalRotation;
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
        m_LocalScale = scl;
        m_Dirty = true;
    }

    void Reset() {
        m_LocalPosition = glm::vec3(0.0f);
        m_LocalEuler = glm::vec3(0.0f);
        m_LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        m_LocalScale = glm::vec3(1.0f);
        m_WorldPositionCache = m_LocalPosition;
        m_WorldRotationCache = m_LocalRotation;
        m_WorldScaleCache = m_LocalScale;
        m_WorldMatrix = glm::mat4(1.0f);
        m_NormalMatrix = glm::mat3(1.0f);
        m_Dirty = true;
    }
    void ResetPosition() {
        m_LocalPosition = glm::vec3(0.0f);
        m_Dirty = true;
    }
    void ResetRotation() {
        m_LocalEuler = glm::vec3(0.0f);
        m_LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        m_Dirty = true;
    }
    void ResetScale() {
        m_LocalScale = glm::vec3(1.0f);
        m_Dirty = true;
    }

    const glm::vec3& GetPosition() const { return m_LocalPosition; }
    const glm::quat& GetRotation() const { return m_LocalRotation; }
    const glm::vec3& GetEulerRotation() const { return m_LocalEuler; }
    const glm::vec3& GetScale() const { return m_LocalScale; }

    glm::vec3 GetWorldPosition() const noexcept { return glm::vec3(m_WorldMatrix[3]); }
    const glm::quat& GetWorldRotation() const noexcept { return m_WorldRotationCache; }
    const glm::vec3& GetWorldScale() const noexcept { return m_WorldScaleCache; }

    void SetLockScaleRatio(bool lock) { m_LockScaleRatio = lock; }
    bool IsScaleRatioLocked() const { return m_LockScaleRatio; }

    bool IsDirty() const { return m_Dirty; }
    void SetDirty(bool dirty = true) { m_Dirty = dirty; }

    void SetWorldMatrix(const glm::mat4& matrix) {
        m_WorldMatrix = matrix;

        glm::mat3 m3(matrix);
        glm::vec3 col0(m3[0]);
        glm::vec3 col1(m3[1]);
        glm::vec3 col2(m3[2]);

        float l0sq = glm::dot(col0, col0);
        float l1sq = glm::dot(col1, col1);
        float l2sq = glm::dot(col2, col2);

        float sx = std::sqrt(l0sq);
        float sy = std::sqrt(l1sq);
        float sz = std::sqrt(l2sq);

        m_WorldPositionCache = glm::vec3(matrix[3]);
        m_WorldScaleCache = glm::vec3(sx, sy, sz);

        glm::mat3 rotMatrix(1.0f);
        if (sx > 1e-6f) rotMatrix[0] = col0 / sx;
        if (sy > 1e-6f) rotMatrix[1] = col1 / sy;
        if (sz > 1e-6f) rotMatrix[2] = col2 / sz;

        float det = glm::determinant(rotMatrix);
        if (det < 0.0f)
        {
            rotMatrix[0] = -rotMatrix[0];
            m_WorldScaleCache.x = -m_WorldScaleCache.x;
        }
        m_WorldRotationCache = glm::quat_cast(rotMatrix);

        bool isOrthogonal = std::abs(glm::dot(rotMatrix[0], rotMatrix[1])) < 1e-3f &&
                            std::abs(glm::dot(rotMatrix[1], rotMatrix[2])) < 1e-3f &&
                            std::abs(glm::dot(rotMatrix[0], rotMatrix[2])) < 1e-3f;

        if (isOrthogonal && l0sq > 1e-6f && l1sq > 1e-6f && l2sq > 1e-6f)
        {
            m_NormalMatrix[0] = col0 / l0sq;
            m_NormalMatrix[1] = col1 / l1sq;
            m_NormalMatrix[2] = col2 / l2sq;
        }
        else
        {
            float m3Det = glm::determinant(m3);
            if (std::abs(m3Det) > 1e-6f)
                m_NormalMatrix = glm::transpose(glm::inverse(m3));
            else
                m_NormalMatrix = glm::mat3(1.0f);
        }

        m_Dirty = false;
    }
    const glm::mat4& GetWorldMatrix() const { return m_WorldMatrix; }
    const glm::mat3& GetNormalMatrix() const { return m_NormalMatrix; }

    inline glm::mat4 GetLocalMatrix() const noexcept
    {
        glm::mat3 rot = glm::mat3_cast(m_LocalRotation);
        glm::mat4 m;
        m[0] = glm::vec4(rot[0] * m_LocalScale.x, 0.0f);
        m[1] = glm::vec4(rot[1] * m_LocalScale.y, 0.0f);
        m[2] = glm::vec4(rot[2] * m_LocalScale.z, 0.0f);
        m[3] = glm::vec4(m_LocalPosition, 1.0f);
        return m;
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
    glm::mat3 m_NormalMatrix = glm::mat3(1.0f);

    glm::vec3 m_WorldPositionCache{ 0.0f };
    glm::quat m_WorldRotationCache{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 m_WorldScaleCache{ 1.0f, 1.0f, 1.0f };

    bool m_Dirty = true;
    bool m_LockScaleRatio = false;
    glm::vec3 m_ScaleRatio = glm::vec3(1.0f);
};

}
