#include "rvelapch.h"
#include "CharacterContactListener.h"
#include "UserData.h"
#include "CollisionFilter.h"
#include "Jolt/Physics/PhysicsSystem.h"

using namespace rv::Physics;

bool CharacterContactListener::OnContactValidate(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2,
    const JPH::SubShapeID& inSubShapeID2)
{
    CollisionFilter a = reinterpret_cast<UserData*>(inCharacter->GetUserData())->filter;
    CollisionFilter b = reinterpret_cast<UserData*>(m_PhysicsSystem->GetBodyInterface().GetUserData(inBodyID2))->filter;

    uint32_t layerA = a.layer;
    uint32_t maskA = a.mask;

    uint32_t layerB = b.layer;
    uint32_t maskB = b.mask;

    if ((maskA & layerB) == 0 || (maskB & layerA) == 0)
        return false;

    return true;
}

bool CharacterContactListener::OnCharacterContactValidate(const JPH::CharacterVirtual* inCharacter,
    const JPH::CharacterVirtual* inOtherCharacter, const JPH::SubShapeID& inSubShapeID2)
{
    CollisionFilter a = reinterpret_cast<UserData*>(inCharacter->GetUserData())->filter;
    CollisionFilter b = reinterpret_cast<UserData*>(inOtherCharacter->GetUserData())->filter;

    uint32_t layerA = a.layer;
    uint32_t maskA = a.mask;

    uint32_t layerB = b.layer;
    uint32_t maskB = b.mask;

    if ((maskA & layerB) == 0 || (maskB & layerA) == 0)
        return false;

    return true;
}


void CharacterContactListener::OnContactAdded(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::BodyID& inBodyID2,
    const JPH::SubShapeID& inSubShapeID2,
    JPH::RVec3Arg inContactPosition,
    JPH::Vec3Arg inContactNormal,
    JPH::CharacterContactSettings& ioSettings)
{
    ioSettings.mCanPushCharacter = true; 
    ioSettings.mCanReceiveImpulses = true; 
}

void CharacterContactListener::OnContactPersisted(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::BodyID& inBodyID2,
    const JPH::SubShapeID& inSubShapeID2,
    JPH::RVec3Arg inContactPosition,
    JPH::Vec3Arg inContactNormal,
    JPH::CharacterContactSettings& ioSettings)
{
}

void CharacterContactListener::OnContactRemoved(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::BodyID& inBodyID2,
    const JPH::SubShapeID& inSubShapeID2)
{
}

void CharacterContactListener::OnContactSolve(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::BodyID& inBodyID2,
    const JPH::SubShapeID& inSubShapeID2,
    JPH::RVec3Arg inContactPosition,
    JPH::Vec3Arg inContactNormal,
    JPH::Vec3Arg inContactVelocity,
    const JPH::PhysicsMaterial* inContactMaterial,
    JPH::Vec3Arg inCharacterVelocity,
    JPH::Vec3& ioNewCharacterVelocity)
{
    // use if not allowed sliding
    //if (inCharacterVelocity.IsNearZero() && !inCharacter->IsSlopeTooSteep(inContactNormal))
        //ioNewCharacterVelocity = JPH::Vec3::sZero();
}

