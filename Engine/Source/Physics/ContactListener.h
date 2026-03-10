#pragma once
#include "Jolt/Jolt.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include "CollisionEvent.h"
#include "ContactKey.h"

namespace rv::Physics {

class ContactListener : public JPH::ContactListener
{
public:
	void Init(JPH::PhysicsSystem* physicsSystem , std::vector<CollisionEvent>* eventQueue) { m_PhysicsSystem = physicsSystem; m_EventQueue = eventQueue; }
	
private:
	virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;
	virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
	virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
	virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;
	
	CollisionEvent BuildEvent(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, const CollisionEventType& eventType);

	JPH::PhysicsSystem* m_PhysicsSystem = nullptr;
	std::vector<CollisionEvent>* m_EventQueue;
	std::unordered_map<ContactKey, CollisionEvent, ContactKeyHash> m_ActiveContacts;
};


}