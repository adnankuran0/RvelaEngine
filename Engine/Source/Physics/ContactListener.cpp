#include "rvelapch.h"
#include "ContactListener.h"
#include "Core/Log.h"
#include "Math/RvelaMath.h"

using namespace rv::Physics;

static entt::entity BodyToEntity(const JPH::Body& body)
{
	return static_cast<entt::entity>(body.GetUserData());
}

JPH::ValidateResult ContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
{
	return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}
void ContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	m_EventQueue->push_back(BuildEvent(inBody1, inBody2, inManifold, CollisionEventType::ENTER));
}

void ContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	m_EventQueue->push_back(BuildEvent(inBody1, inBody2, inManifold, CollisionEventType::STAY));
}

void ContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
{
	/*JPH::BodyID idA = inSubShapePair.GetBody1ID();
	JPH::BodyID idB = inSubShapePair.GetBody2ID();
	entt::entity b = static_cast<entt::entity>(m_BodyInterface->GetUserData(idB));
	entt::entity a = static_cast<entt::entity>(m_BodyInterface->GetUserData(idA));
	CollisionEvent event;
	event.entityA = a;
	event.entityB = b;
	event.eventType = CollisionEventType::EXIT;

	m_EventQueue->push_back(event);*/
}

CollisionEvent ContactListener::BuildEvent(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, const CollisionEventType& eventType)
{
	CollisionEvent event;
	event.entityA = BodyToEntity(inBody1);
	event.entityB = BodyToEntity(inBody2);
	event.eventType = eventType;

	JPH::Vec3 localPoint = inManifold.mRelativeContactPointsOn1[0];
	JPH::RVec3 worldPoint = inManifold.mBaseOffset + localPoint;
	event.collisionInfo.point = math::FromJoltRVec3(worldPoint);
	event.collisionInfo.normal = math::FromJoltVec3(inManifold.mWorldSpaceNormal);
	return event;
}
