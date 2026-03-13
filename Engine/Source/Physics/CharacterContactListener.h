#pragma once
#include "Jolt/Jolt.h"
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include "CollisionEvent.h"
#include "CharacterContactKey.h"

namespace rv::Physics {

class CharacterContactListener : public JPH::CharacterContactListener
{
public:
    void Init(JPH::PhysicsSystem* physicsSystem, std::vector<CollisionEvent>* eventQueue) { m_PhysicsSystem = physicsSystem; m_EventQueue = eventQueue; }

    bool OnContactValidate(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, 
        const JPH::SubShapeID& inSubShapeID2) override;

    bool OnCharacterContactValidate(const JPH::CharacterVirtual* inCharacter, 
        const JPH::CharacterVirtual* inOtherCharacter, const JPH::SubShapeID& inSubShapeID2);

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
private:
    CollisionEvent BuildEvent(
        const JPH::CharacterVirtual* inCharacter,
        const JPH::BodyID& inBodyID2,
        JPH::RVec3Arg inContactPosition,
        JPH::Vec3Arg inContactNormal,
        CollisionEventType eventType);

    JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
    std::mutex m_EventMutex;
    std::vector<CollisionEvent>* m_EventQueue = nullptr;
    std::unordered_map<CharacterContactKey, CollisionEvent, CharacterContactKeyHash> m_ActiveContacts;
};

} 