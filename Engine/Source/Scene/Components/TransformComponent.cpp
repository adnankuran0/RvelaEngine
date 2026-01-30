#include "rvelapch.h"
#include "TransformComponent.h"

void TransformComponent::LookAt(const glm::vec3& target, const glm::vec3& up) noexcept
{
    glm::mat4 lookAt = glm::lookAt(localPosition, target, up);
    glm::mat3 rotMat = glm::mat3(lookAt);
    localRotation = glm::quat_cast(glm::transpose(rotMat));
    localEuler = QuatToEuler(localRotation);
    dirty = true;
}

json TransformComponent::Serialize() const noexcept
{
    json j;
    j["position"] = { localPosition.x, localPosition.y, localPosition.z };
    j["rotation"] = { localEuler.x, localEuler.y, localEuler.z };
    j["scale"] = { localScale.x, localScale.y, localScale.z };
    j["lockScaleRatio"] = lockScaleRatio;
    j["scaleRatio"] = { scaleRatio.x, scaleRatio.y, scaleRatio.z };
    return j;
}
void TransformComponent::Deserialize(const json& j) noexcept
{
    auto& p = j.at("position");
    auto& r = j.at("rotation");
    auto& s = j.at("scale");
    auto& lsr = j.at("lockScaleRatio");
    auto& sr = j.at("scaleRatio");

    localPosition = glm::vec3(p[0], p[1], p[2]);
    SetEulerRotation(glm::vec3(r[0], r[1], r[2]));
    localScale = glm::vec3(s[0], s[1], s[2]);
    lockScaleRatio = lsr.get<bool>();
    scaleRatio = glm::vec3(sr[0], sr[1], sr[2]);

    dirty = true;
}