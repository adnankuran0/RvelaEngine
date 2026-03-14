#include "rvelapch.h"
#include "ContactListener.h"
#include "Core/Log.h"
#include "Math/RvelaMath.h"
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include "Scene/Scene.h"
#include "UserData.h"

using namespace rv::Physics;

static entt::entity BodyToEntity(const JPH::Body& body)
{
	return reinterpret_cast<UserData*>(body.GetUserData())->entity;
}

JPH::ValidateResult ContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, 
	JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
{

	CollisionFilter a = reinterpret_cast<UserData*>(inBody1.GetUserData())->filter;
	CollisionFilter b = reinterpret_cast<UserData*>(inBody2.GetUserData())->filter;

	uint32_t layerA = a.layer;
	uint32_t maskA = a.mask;

	uint32_t layerB = b.layer;
	uint32_t maskB = b.mask;

	if ((maskA & layerB) == 0 || (maskB & layerA) == 0)
		return JPH::ValidateResult::RejectContact;

	return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

void ContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
	const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	CollisionEvent e = BuildEvent(inBody1, inBody2, inManifold, CollisionEventType::ENTER);

	ContactKey key(inBody1.GetID(), inBody2.GetID());
	m_ActiveContacts[key] = e;

	std::lock_guard lock(m_EventMutex);
	m_EventQueue->push_back(e);
}

void ContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, 
	const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	CollisionEvent e = BuildEvent(inBody1, inBody2, inManifold, CollisionEventType::STAY);

	ContactKey key(inBody1.GetID(), inBody2.GetID());
	m_ActiveContacts[key] = e;

	std::lock_guard lock(m_EventMutex);
	m_EventQueue->push_back(e);
}

void ContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
{
	ContactKey key(inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID());

	auto it = m_ActiveContacts.find(key);
	if (it == m_ActiveContacts.end())
		return;

	CollisionEvent e = it->second;
	e.eventType = CollisionEventType::EXIT;

	std::lock_guard lock(m_EventMutex);
	m_EventQueue->push_back(e);

	m_ActiveContacts.erase(it);
}

CollisionEvent ContactListener::BuildEvent(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, const CollisionEventType& eventType)
{
	CollisionEvent event;
	event.entityA = BodyToEntity(inBody1);
	event.entityB = BodyToEntity(inBody2);
	event.eventType = eventType;

	JPH::Vec3 localPoint = inManifold.mRelativeContactPointsOn1[0];
	JPH::RVec3 worldPoint = inManifold.mBaseOffset + localPoint;
	event.collision.point = math::FromJoltRVec3(worldPoint);
	event.collision.normal = math::FromJoltVec3(inManifold.mWorldSpaceNormal);
	return event;
}
