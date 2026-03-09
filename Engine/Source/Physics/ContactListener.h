#pragma once
#include "Jolt/Jolt.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include "CollisionEvent.h"


namespace rv::Physics {

class ContactListener : public JPH::ContactListener
{
public:
	void Init(JPH::BodyInterface* bodyInterface, std::vector<CollisionEvent>* eventQueue) { m_BodyInterface = bodyInterface; m_EventQueue = eventQueue; }
	
private:
	virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;
	virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
	virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
	virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;
	
	CollisionEvent BuildEvent(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, const CollisionEventType& eventType);

	JPH::BodyInterface* m_BodyInterface = nullptr;
	std::vector<CollisionEvent>* m_EventQueue;
};


}