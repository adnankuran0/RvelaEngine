#pragma once
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Math/RvelaMath.h"
#include "json.hpp"

namespace rv {

using json = nlohmann::json;

struct alignas(16) TransformComponent
{


public:
    TransformComponent() = default;
    TransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
        : localPosition(pos), localEuler(rot), localScale(scl) 
    {
        localRotation = math::EulerToQuat(localEuler);
        dirty = true;
    }

    inline void SetPosition(const glm::vec3& pos) noexcept { localPosition = pos; dirty = true; }
    inline glm::vec3 GetPosition() const noexcept { return localPosition; }

    inline void SetRotation(const glm::quat& quat) noexcept { localRotation = quat; localEuler = math::QuatToEuler(quat); dirty = true; }
    inline glm::quat GetRotation() const noexcept { return localRotation; }

    inline void SetEulerRotation(const glm::vec3& euler) noexcept { localEuler = euler; localRotation = math::EulerToQuat(euler); dirty = true; }
    inline glm::vec3 GetEulerRotation() const noexcept { return localEuler; }

    inline const glm::vec3& GetWorldPosition() const noexcept { return worldPosition; }
    inline const glm::quat& GetWorldRotation() const noexcept { return worldRotation; }
    inline const glm::vec3& GetWorldScale() const noexcept { return worldScale; }


    inline void SetScale(const glm::vec3& newScale) noexcept
    {
        if (lockScaleRatio) 
        {
            float uniform = newScale.x;
            localScale = scaleRatio * uniform;
        }
        else 
        {
            localScale = newScale;
        }
        dirty = true;
    }
    inline glm::vec3 GetScale() const noexcept { return localScale; }

    inline void SetLockScaleRatio(bool lock) noexcept
    {
        lockScaleRatio = lock;
        if (lock) 
        {
            if (localScale.x != 0.0f)
                scaleRatio = localScale / localScale.x;
            else
                scaleRatio = glm::vec3(1.0f);
        }
    }
    inline bool IsScaleRatioLocked() const noexcept { return lockScaleRatio; }

    inline bool IsDirty() const noexcept { return dirty; }
    inline void SetDirty() noexcept { dirty = true; }
    inline void ClearDirty() noexcept { dirty = false; }

    inline glm::mat4 GetLocalMatrix() const noexcept
    {
        return glm::translate(glm::mat4(1.0f), localPosition)
            * glm::mat4_cast(localRotation)
            * glm::scale(glm::mat4(1.0f), localScale);
    }

    inline glm::mat4 GetWorldMatrix() const noexcept
    {
        return glm::translate(glm::mat4(1.0f), worldPosition)
            * glm::mat4_cast(worldRotation)
            * glm::scale(glm::mat4(1.0f), worldScale);
    }

    inline glm::vec3 GetForward() const noexcept
    {
        return glm::normalize(glm::mat3_cast(worldRotation) * glm::vec3(0.0f, 0.0f, -1.0f));
    }
    inline glm::vec3 GetUp() const noexcept
    {
        return glm::normalize(glm::mat3_cast(worldRotation) * glm::vec3(0.0f, 1.0f, 0.0f));
    }
    inline glm::vec3 GetRight() const noexcept
    {
        return glm::normalize(glm::mat3_cast(worldRotation) * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    inline void Translate(const glm::vec3& delta) noexcept { localPosition += delta; dirty = true; }

    void LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0)) noexcept;

    inline void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl) noexcept
    {
        worldPosition = pos;
        worldRotation = rot;
        worldScale = scl;
    }

    json Serialize() const noexcept;
    void Deserialize(const json& j) noexcept;

private:
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::quat worldRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::vec3 localPosition = glm::vec3(0.0f);
    glm::vec3 worldPosition = glm::vec3(0.0f);
    glm::vec3 localScale = glm::vec3(1.0f);
    glm::vec3 worldScale = glm::vec3(1.0f);
    glm::vec3 scaleRatio = glm::vec3(1.0f);
    glm::vec3 localEuler = glm::vec3(0.0f);
    glm::vec3 worldEuler = glm::vec3(0.0f);

    bool lockScaleRatio = false;
    bool dirty = true;
};


}