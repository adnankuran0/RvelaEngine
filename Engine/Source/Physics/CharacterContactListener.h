#pragma once
#include "Jolt/Jolt.h"
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace rv::Physics {

class CharacterContactListener : public JPH::CharacterContactListener
{
public:
    void OnContactAdded(
        const JPH::CharacterVirtual* inCharacter,
        const JPH::BodyID& inBodyID2,
        const JPH::SubShapeID& inSubShapeID2,
        JPH::RVec3Arg inContactPosition,
        JPH::Vec3Arg inContactNormal,
        JPH::CharacterContactSettings& ioSettings) override;

    void OnContactPersisted(
        const JPH::CharacterVirtual* inCharacter,
        const JPH::BodyID& inBodyID2,
        const JPH::SubShapeID& inSubShapeID2,
        JPH::RVec3Arg inContactPosition,
        JPH::Vec3Arg inContactNormal,
        JPH::CharacterContactSettings& ioSettings) override;

    void OnContactRemoved(
        const JPH::CharacterVirtual* inCharacter,
        const JPH::BodyID& inBodyID2,
        const JPH::SubShapeID& inSubShapeID2) override;

    void OnContactSolve(
        const JPH::CharacterVirtual* inCharacter,
        const JPH::BodyID& inBodyID2,
        const JPH::SubShapeID& inSubShapeID2,
        JPH::RVec3Arg inContactPosition,
        JPH::Vec3Arg inContactNormal,
        JPH::Vec3Arg inContactVelocity,
        const JPH::PhysicsMaterial* inContactMaterial,
        JPH::Vec3Arg inCharacterVelocity,
        JPH::Vec3& ioNewCharacterVelocity) override;
};

} 