#include "rvelapch.h"
#include "CharacterContactListener.h"
using namespace rv::Physics;

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

