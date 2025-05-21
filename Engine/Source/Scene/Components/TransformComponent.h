#pragma once
#include "Scene/Component.h"
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Core/RvelaMath.h"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

class TransformComponent : public Component {
public:
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation;
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 rotation_degrees = glm::vec3(0.0f);


    TransformComponent() = default;
    TransformComponent(glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
        : position(pos), rotation(rot), scale(scl) {
    }

    void SetEulerRotation(const glm::vec3& euler) {
        rotation_degrees = euler;
        rotation = EulerToQuat(rotation_degrees);
    }
    glm::vec3 GetEulerRotation() const {
        return rotation_degrees;
    }

    void SetRotation(const glm::quat& quat)
    {
        rotation = quat;
        rotation_degrees = QuatToEuler(quat);
    }

    glm::mat4 GetMatrix() const {
        return glm::translate(glm::mat4(1.0f), position) *
            glm::mat4_cast(rotation) *
            glm::scale(glm::mat4(1.0f), scale);
    }

    glm::vec3 GetForward() const {
        return glm::normalize(glm::mat3_cast(rotation) * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 GetUp() const {
        return glm::normalize(glm::mat3_cast(rotation) * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 GetRight() const {
        return glm::normalize(glm::mat3_cast(rotation) * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    std::string Serialize() const override
    {
        json j;
        j["position"] = { position.x,position.y,position.z };
        j["rotation"] = { rotation_degrees.x, rotation_degrees.y, rotation_degrees.z };
        j["scale"] = { scale.x,scale.y,scale.z };

        return j.dump(4);
    }
    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto positionData = j["position"];
        position = glm::vec3(positionData[0], positionData[1], positionData[2]);
        auto rotationData = j["rotation"];
        glm::vec3 euler(rotationData[0], rotationData[1], rotationData[2]);
        SetEulerRotation(euler);
        auto scaleData = j["scale"];
        scale = glm::vec3(scaleData[0], scaleData[1], scaleData[2]);

    }
};