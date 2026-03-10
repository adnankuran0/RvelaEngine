#include "rvelapch.h"
#include "CharacterBodyComponent.h"

using namespace rv;

json CharacterBodyComponent::Serialize() const
{
    json j;
    j["mass"] = mass;
    j["maxStrength"] = maxStrength;
    j["predictiveContactDistance"] = predictiveContactDistance;
    j["shapeOffset"] = { shapeOffset.x, shapeOffset.y, shapeOffset.z };
    j["maxSlopeAngle"] = maxSlopeAngle;

    j["collisionLayer"] = collisionFilter.layer;
    j["collisionMask"] = collisionFilter.mask;

    return j;
}

void CharacterBodyComponent::Deserialize(const json& j)
{
    mass = j["mass"];
    maxStrength = j["maxStrength"];
    auto offsetData = j.at("shapeOffset");
    shapeOffset = glm::vec3(offsetData[0], offsetData[1], offsetData[2]);
    predictiveContactDistance = j["predictiveContactDistance"];
    maxSlopeAngle = j["maxSlopeAngle"];

    collisionFilter.layer = j.value("collisionLayer", 1u);
    collisionFilter.mask = j.value("collisionMask", 0xFFFFFFFFu);
}
