#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include "Physics/MotionType.h"
#include "../nlohmann/json.hpp"

namespace rv {

using json = nlohmann::json;

struct RigidbodyComponent 
{
    JPH::BodyID RuntimeBodyID{};

    Physics::MotionType bodyType = Physics::MotionType::STATIC;

    bool autoCalculateMass = true;
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.0f;
    float linearDamping = 0.05f; 
    float angularDamping = 0.05f;
    float maxLinearVelocity = 500.0f;
    float maxAngularVelocity = 50.0f;
    bool allowSleep = true;

    bool lockRotationX = false;
    bool lockRotationY = false;
    bool lockRotationZ = false;

    bool lockTranslationX = false;
    bool lockTranslationY = false;
    bool lockTranslationZ = false;

    float gravityFactor = 1.0f;

    bool isSensor = false;

    bool useCCD = false;

    bool interpolationReady = false;
    glm::vec3 previousPosition{};
    glm::quat previousRotation{ 1,0,0,0 };
    glm::vec3 currentPosition{};
    glm::quat currentRotation{ 1,0,0,0 };

    json Serialize() const;
    void Deserialize(const json& j);
};

}