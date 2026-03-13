#include "rvelapch.h"
#include "CharacterContactListener.h"
#include "UserData.h"
#include "CollisionFilter.h"
#include "Math/RvelaMath.h"
#include "Jolt/Physics/PhysicsSystem.h"

using namespace rv::Physics;

static entt::entity CharacterToEntity(const JPH::CharacterVirtual* character)
{
    return reinterpret_cast<UserData*>(character->GetUserData())->entity;
}

static entt::entity BodyToEntity(JPH::PhysicsSystem* physicsSystem, const JPH::BodyID& bodyID)
{
    return reinterpret_cast<UserData*>(
        physicsSystem->GetBodyInterface().GetUserData(bodyID)
        )->entity;
}

bool CharacterContactListener::OnContactValidate(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2,
    const JPH::SubShapeID& inSubShapeID2)
{
    CollisionFilter a = reinterpret_cast<UserData*>(inCharacter->GetUserData())->filter;
    CollisionFilter b = reinterpret_cast<UserData*>(m_PhysicsSystem->GetBodyInterface().GetUserData(inBodyID2))->filter;

    if ((a.mask & b.layer) == 0 || (b.mask & a.layer) == 0)
        return false;
    return true;
}

bool CharacterContactListener::OnCharacterContactValidate(const JPH::CharacterVirtual* inCharacter,
    const JPH::CharacterVirtual* inOtherCharacter, const JPH::SubShapeID& inSubShapeID2)
{
    CollisionFilter a = reinterpret_cast<UserData*>(inCharacter->GetUserData())->filter;
    CollisionFilter b = reinterpret_cast<UserData*>(inOtherCharacter->GetUserData())->filter;

    if ((a.mask & b.layer) == 0 || (b.mask & a.layer) == 0)
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

    CollisionEvent e = BuildEvent(inCharacter, inBodyID2, inContactPosition, inContactNormal, CollisionEventType::ENTER);

    CharacterContactKey key{ inCharacter, inBodyID2 };
    m_ActiveContacts[key] = e;

    std::lock_guard lock(m_EventMutex);
    m_EventQueue->push_back(e);
}

void CharacterContactListener::OnContactPersisted(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::BodyID& inBodyID2,
    const JPH::SubShapeID& inSubShapeID2,
    JPH::RVec3Arg inContactPosition,
    JPH::Vec3Arg inContactNormal,
    JPH::CharacterContactSettings& ioSettings)
{
    CollisionEvent e = BuildEvent(inCharacter, inBodyID2, inContactPosition, inContactNormal, CollisionEventType::STAY);

    CharacterContactKey key{ inCharacter, inBodyID2 };
    m_ActiveContacts[key] = e;

    std::lock_guard lock(m_EventMutex);
    m_EventQueue->push_back(e);
}

void CharacterContactListener::OnContactRemoved(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::BodyID& inBodyID2,
    const JPH::SubShapeID& inSubShapeID2)
{
    CharacterContactKey key{ inCharacter, inBodyID2 };
    auto it = m_ActiveContacts.find(key);
    if (it == m_ActiveContacts.end())
        return;

    CollisionEvent e = it->second;
    e.eventType = CollisionEventType::EXIT;

    std::lock_guard lock(m_EventMutex);
    m_EventQueue->push_back(e);
    m_ActiveContacts.erase(it);
}

CollisionEvent CharacterContactListener::BuildEvent(
    const JPH::CharacterVirtual* inCharacter,
    const JPH::BodyID& inBodyID2,
    JPH::RVec3Arg inContactPosition,
    JPH::Vec3Arg inContactNormal,
    CollisionEventType eventType)
{
    CollisionEvent event;
    event.entityA = CharacterToEntity(inCharacter);
    event.entityB = BodyToEntity(m_PhysicsSystem, inBodyID2);
    event.eventType = eventType;
    event.collision.point = math::FromJoltRVec3(inContactPosition);
    event.collision.normal = math::FromJoltVec3(inContactNormal);
    return event;
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

