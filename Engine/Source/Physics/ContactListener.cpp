#include "rvelapch.h"
#include "ContactListener.h"
#include "Core/Log.h"

using namespace rv::Physics;

JPH::ValidateResult ContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult)
{
	LOG_DEBUG("Contact validate callback");

	return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}
void ContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	LOG_DEBUG("A contact was added");
}

void ContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
	LOG_DEBUG("A contact was persisted");
}

void ContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
{
	LOG_DEBUG("A contact was removed");
}
