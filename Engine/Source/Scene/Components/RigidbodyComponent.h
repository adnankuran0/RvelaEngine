#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include "Physics/BodyType.h"
#include "../nlohmann/json.hpp"

namespace rv {

using json = nlohmann::json;

struct RigidbodyComponent 
{
    JPH::BodyID RuntimeBodyID{};

    Physics::BodyType bodyType = Physics::BodyType::DYNAMIC;

    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.0f;
    float linearDamping = 0.05f; 
    float angularDamping = 0.05f;

    bool lockRotationX = false;
    bool lockRotationY = false;
    bool lockRotationZ = false;

    bool lockTranslationX = false;
    bool lockTranslationY = false;
    bool lockTranslationZ = false;

    float gravityFactor = 1.0f;

    bool isSensor = false;

    bool useCCD = false;

    json Serialize() const;
    void Deserialize(const json& j);
};

}