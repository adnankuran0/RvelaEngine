#pragma once
#include "Scene/Component.h"
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Math/RvelaMath.h"
#include "json.hpp"
using json = nlohmann::json;

class TransformComponent : public Component {
private:
    glm::vec3 localPosition = glm::vec3(0.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 localScale = glm::vec3(1.0f);
    glm::vec3 localEuler = glm::vec3(0.0f);

    glm::vec3 worldPosition = glm::vec3(0.0f);
    glm::quat worldRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 worldScale = glm::vec3(1.0f);

    bool lockScaleRatio = false;
    glm::vec3 scaleRatio = glm::vec3(1.0f);
    bool dirty = true;

public:
    TransformComponent() = default;
    TransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
        : localPosition(pos), localEuler(rot), localScale(scl) {
        localRotation = EulerToQuat(localEuler);
        dirty = true;
    }

    void SetPosition(const glm::vec3& pos) { localPosition = pos; dirty = true; }
    glm::vec3 GetPosition() const { return localPosition; }

    void SetRotation(const glm::quat& quat) { localRotation = quat; localEuler = QuatToEuler(quat); dirty = true; }
    glm::quat GetRotation() const { return localRotation; }

    void SetEulerRotation(const glm::vec3& euler) { localEuler = euler; localRotation = EulerToQuat(euler); dirty = true; }
    glm::vec3 GetEulerRotation() const { return localEuler; }

    void SetScale(const glm::vec3& newScale) {
        if (lockScaleRatio) {
            float uniform = newScale.x;
            localScale = scaleRatio * uniform;
        }
        else {
            localScale = newScale;
        }
        dirty = true;
    }
    glm::vec3 GetScale() const { return localScale; }

    void SetLockScaleRatio(bool lock) {
        lockScaleRatio = lock;
        if (lock) {
            if (localScale.x != 0.0f)
                scaleRatio = localScale / localScale.x;
            else
                scaleRatio = glm::vec3(1.0f);
        }
    }
    bool IsScaleRatioLocked() const { return lockScaleRatio; }

    bool IsDirty() const { return dirty; }
    void SetDirty() { dirty = true; }
    void ClearDirty() { dirty = false; }

    glm::mat4 GetLocalMatrix() const {
        return glm::translate(glm::mat4(1.0f), localPosition)
            * glm::mat4_cast(localRotation)
            * glm::scale(glm::mat4(1.0f), localScale);
    }

    glm::mat4 GetWorldMatrix() const {
        return glm::translate(glm::mat4(1.0f), worldPosition)
            * glm::mat4_cast(worldRotation)
            * glm::scale(glm::mat4(1.0f), worldScale);
    }

    glm::vec3 GetForward() const {
        return glm::normalize(glm::mat3_cast(worldRotation) * glm::vec3(0.0f, 0.0f, -1.0f));
    }
    glm::vec3 GetUp() const {
        return glm::normalize(glm::mat3_cast(worldRotation) * glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glm::vec3 GetRight() const {
        return glm::normalize(glm::mat3_cast(worldRotation) * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    void Translate(const glm::vec3& delta) { localPosition += delta; dirty = true; }

    void LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0)) {
        glm::mat4 lookAt = glm::lookAt(localPosition, target, up);
        glm::mat3 rotMat = glm::mat3(lookAt);
        localRotation = glm::quat_cast(glm::transpose(rotMat));
        localEuler = QuatToEuler(localRotation);
        dirty = true;
    }

    void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl) {
        worldPosition = pos;
        worldRotation = rot;
        worldScale = scl;
    }
    const glm::vec3& GetWorldPosition() const { return worldPosition; }
    const glm::quat& GetWorldRotation() const { return worldRotation; }
    const glm::vec3& GetWorldScale() const { return worldScale; }

    std::string Serialize() const override {
        json j;
        j["position"] = { localPosition.x, localPosition.y, localPosition.z };
        j["rotation"] = { localEuler.x, localEuler.y, localEuler.z };
        j["scale"] = { localScale.x, localScale.y, localScale.z };
        j["lockScaleRatio"] = lockScaleRatio;
        j["scaleRatio"] = { scaleRatio.x, scaleRatio.y, scaleRatio.z };
        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override {
        json j = json::parse(jsonStr);
        auto p = j["position"];
        auto r = j["rotation"];
        auto s = j["scale"];
        auto lsr = j["lockScaleRatio"];
        auto sr = j["scaleRatio"];

        localPosition = glm::vec3(p[0], p[1], p[2]);
        SetEulerRotation(glm::vec3(r[0], r[1], r[2]));
        localScale = glm::vec3(s[0], s[1], s[2]);
        lockScaleRatio = lsr;
        scaleRatio = glm::vec3(sr[0], sr[1], sr[2]);

        dirty = true;
    }
};
