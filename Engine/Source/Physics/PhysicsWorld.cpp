#include "rvelapch.h"
#include "PhysicsWorld.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include "Math/RvelaMath.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include <Jolt/Physics/Collision/CastResult.h>
#include "Jolt/Physics/Collision/CollisionCollectorImpl.h"
#include "UserData.h"

using namespace rv::Physics;

//Rigidbody API

void PhysicsWorld::SetPosition(RigidbodyComponent* comp, const glm::vec3& position)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetPosition(comp->RuntimeBodyID, math::ToJoltVec3(position), JPH::EActivation::Activate);
}

void PhysicsWorld::SetRotation(RigidbodyComponent* comp, const glm::quat& rotation)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetRotation(comp->RuntimeBodyID, math::ToJoltQuat(rotation), JPH::EActivation::Activate);
}

void rv::Physics::PhysicsWorld::MoveKinematic(RigidbodyComponent* comp, 
	const glm::vec3& targetPosition,const glm::quat& targetRotation, float dt)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;
	if (GetMotionType(comp) != MotionType::KINEMATIC) return;

	BodyInterface().MoveKinematic(comp->RuntimeBodyID, 
		math::ToJoltRVec3(targetPosition), 
		math::ToJoltQuat(targetRotation), dt);

}

void PhysicsWorld::SetMotionType(RigidbodyComponent* comp, MotionType motionType)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetMotionType(comp->RuntimeBodyID, 
		static_cast<JPH::EMotionType>(static_cast<int>(motionType)), 
		JPH::EActivation::Activate);
}

MotionType PhysicsWorld::GetMotionType(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return MotionType::STATIC;

	return static_cast<MotionType>(static_cast<int>(BodyInterface().GetMotionType(comp->RuntimeBodyID)));
}

void PhysicsWorld::AddForce(RigidbodyComponent* comp, const glm::vec3& force)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().AddForce(comp->RuntimeBodyID, math::ToJoltVec3(force));
}

void PhysicsWorld::AddImpulse(RigidbodyComponent* comp, const glm::vec3& impulse)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().AddImpulse(comp->RuntimeBodyID, math::ToJoltVec3(impulse));
}

void PhysicsWorld::AddTorque(RigidbodyComponent* comp, const glm::vec3& torque)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().AddTorque(comp->RuntimeBodyID, math::ToJoltVec3(torque));
}

void PhysicsWorld::AddAngularImpulse(RigidbodyComponent* comp, const glm::vec3& angularImpulse)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().AddAngularImpulse(comp->RuntimeBodyID, math::ToJoltVec3(angularImpulse));
}

void PhysicsWorld::AddLinearVelocity(RigidbodyComponent* comp, const glm::vec3& linearVelocity)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().AddLinearVelocity(comp->RuntimeBodyID, math::ToJoltVec3(linearVelocity));
}

void PhysicsWorld::SetLinearVelocity(RigidbodyComponent* comp, const glm::vec3& linearVelocity)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetLinearVelocity(comp->RuntimeBodyID, math::ToJoltVec3(linearVelocity));
}

glm::vec3 PhysicsWorld::GetLinearVelocity(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return glm::vec3(0.0);

	return math::FromJoltVec3(BodyInterface().GetLinearVelocity(comp->RuntimeBodyID));
}

void rv::Physics::PhysicsWorld::SetMaxLinearVelocity(RigidbodyComponent* comp, float maxLinearVelocity)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetMaxLinearVelocity(comp->RuntimeBodyID, maxLinearVelocity);
}

float rv::Physics::PhysicsWorld::GetMaxLinearVelocity(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return 0.0f;

	return BodyInterface().GetMaxLinearVelocity(comp->RuntimeBodyID);
}

void PhysicsWorld::SetAngularVelocity(RigidbodyComponent* comp, const glm::vec3& angularVelociry)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetAngularVelocity(comp->RuntimeBodyID, math::ToJoltVec3(angularVelociry));

}

glm::vec3 PhysicsWorld::GetAngularVelocity(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return glm::vec3(0.0);

	return math::FromJoltVec3(BodyInterface().GetAngularVelocity(comp->RuntimeBodyID));
}

void rv::Physics::PhysicsWorld::SetMaxAngularVelocity(RigidbodyComponent* comp, float maxAngularVelocityInRadians)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetMaxAngularVelocity(comp->RuntimeBodyID, maxAngularVelocityInRadians);
}

float rv::Physics::PhysicsWorld::GetMaxAngularVelocity(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return 0.0f;

	return BodyInterface().GetMaxAngularVelocity(comp->RuntimeBodyID);
}

glm::vec3 PhysicsWorld::GetCenterOfMassPosition(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return glm::vec3(0.0);

	return math::FromJoltVec3(BodyInterface().GetCenterOfMassPosition(comp->RuntimeBodyID));
}

void PhysicsWorld::SetFriction(RigidbodyComponent* comp, float friction)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	return BodyInterface().SetFriction(comp->RuntimeBodyID, friction);
}

float PhysicsWorld::GetFriction(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return 0.0f;

	return BodyInterface().GetFriction(comp->RuntimeBodyID);
}

void PhysicsWorld::SetGravityFactor(RigidbodyComponent* comp, float gravityFactor)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetGravityFactor(comp->RuntimeBodyID, gravityFactor);
}

float rv::Physics::PhysicsWorld::GetGravityFactor(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return 0.0f;

	return BodyInterface().GetGravityFactor(comp->RuntimeBodyID);
}

void PhysicsWorld::SetRestitution(RigidbodyComponent* comp, float restitution)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetRestitution(comp->RuntimeBodyID, restitution);
}

float PhysicsWorld::GetRestitution(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return 0.0f;

	return BodyInterface().GetRestitution(comp->RuntimeBodyID);
}

void PhysicsWorld::SetIsSensor(RigidbodyComponent* comp, bool isSensor)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetIsSensor(comp->RuntimeBodyID, isSensor);
}

bool PhysicsWorld::IsSensor(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return false;

	return BodyInterface().IsSensor(comp->RuntimeBodyID);
}

// CharacterBody API

bool PhysicsWorld::IsCharacterGrounded(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return false;

	return comp->character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
}

bool PhysicsWorld::IsCharacterOnSteepGround(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return false;

	return comp->character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnSteepGround;
}

bool PhysicsWorld::IsCharacterInAir(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return false;

	return comp->character->GetGroundState() == JPH::CharacterVirtual::EGroundState::InAir;
}

bool PhysicsWorld::IsCharacterNotSupported(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return false;

	return comp->character->GetGroundState() == JPH::CharacterVirtual::EGroundState::NotSupported;
}

glm::vec3 PhysicsWorld::GetCharacterPosition(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return glm::vec3(0.0f);

	return math::FromJoltVec3(comp->character->GetPosition());
}

void PhysicsWorld::SetCharacterPosition(CharacterBodyComponent* comp, const glm::vec3& position)
{
	if (!comp || !comp->character) return;

	comp->character->SetPosition(math::ToJoltRVec3(position));
}

glm::quat PhysicsWorld::GetCharacterRotation(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	return math::FromJoltQuat(comp->character->GetRotation());
}

void PhysicsWorld::SetCharacterRotation(CharacterBodyComponent* comp, const glm::quat& rotation)
{
	if (!comp || !comp->character) return;

	comp->character->SetRotation(math::ToJoltQuat(rotation));
}

glm::vec3 PhysicsWorld::GetCharacterCenterOfMassPosition(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return glm::vec3(0.0f);

	return math::FromJoltVec3(comp->character->GetCenterOfMassPosition());
}

glm::vec3 PhysicsWorld::GetCharacterLinearVelocity(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return glm::vec3(0.0f);

	return math::FromJoltVec3(comp->character->GetLinearVelocity());
}

void PhysicsWorld::SetCharacterLinearVelocity(CharacterBodyComponent* comp, const glm::vec3& velocity)
{
	if (!comp || !comp->character) return;

	comp->character->SetLinearVelocity(math::ToJoltVec3(velocity));
}

glm::vec3 PhysicsWorld::GetCharacterUp(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return glm::vec3(0.0f, 1.0f, 0.0f);

	return math::FromJoltVec3(comp->character->GetUp());
}

void PhysicsWorld::SetCharacterUp(CharacterBodyComponent* comp, const glm::vec3& up)
{
	if (!comp || !comp->character) return;

	comp->character->SetUp(math::ToJoltVec3(up));
}

float PhysicsWorld::GetCharacterMaxSlopeAngle(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return 0.0f;

	return comp->character->GetCosMaxSlopeAngle();
}

void PhysicsWorld::SetCharacterMaxSlopeAngle(CharacterBodyComponent* comp, float angleInRadians)
{
	if (!comp || !comp->character) return;

	comp->character->SetMaxSlopeAngle(angleInRadians);
}

glm::vec3 PhysicsWorld::GetCharacterGroundNormal(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return glm::vec3(0.0f, 1.0f, 0.0f);

	return math::FromJoltVec3(comp->character->GetGroundNormal());
}

glm::vec3 PhysicsWorld::GetCharacterGroundVelocity(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return glm::vec3(0.0f);

	return math::FromJoltVec3(comp->character->GetGroundVelocity());
}

glm::vec3 PhysicsWorld::GetCharacterGroundPosition(CharacterBodyComponent* comp)
{
	if (!comp || !comp->character) return glm::vec3(0.0f);

	return math::FromJoltVec3(comp->character->GetGroundPosition());
}

RaycastResult PhysicsWorld::Raycast(const glm::vec3& rayOrigin,
	const glm::vec3& rayDirection,
	float maxDistance,
	bool hitInside)
{
	RaycastResult result;
	glm::vec3 dir = glm::normalize(rayDirection);
	JPH::RRayCast ray(
		math::ToJoltRVec3(rayOrigin),
		math::ToJoltVec3(dir * maxDistance)
	);

	JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
	JPH::RayCastSettings settings;
	settings.mTreatConvexAsSolid = false;

	m_PhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, settings, collector);

	if (!collector.HadHit())
		return result;

	const JPH::RayCastResult* bestHit = nullptr;
	float bestFraction = FLT_MAX;

	for (const auto& hit : collector.mHits)
	{
		if (!hitInside && hit.mFraction <= 0.0001f) continue; // skip if inside
		if (hit.mFraction < bestFraction)
		{
			bestFraction = hit.mFraction;
			bestHit = &hit;
		}

	}

	if (!bestHit)
		return result;

	float distance = bestHit->mFraction * maxDistance;
	JPH::RVec3 hitPoint = ray.mOrigin + ray.mDirection * bestHit->mFraction;

	const JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), bestHit->mBodyID);
	if (lock.Succeeded())
	{
		const JPH::Body& body = lock.GetBody();
		JPH::Vec3 normal = body.GetWorldSpaceSurfaceNormal(bestHit->mSubShapeID2, hitPoint);
		result.hit = true;
		result.distance = distance;
		result.point = math::FromJoltRVec3(hitPoint);
		result.normal = math::FromJoltVec3(normal);
		result.entity = reinterpret_cast<UserData*>(body.GetUserData())->entity;
	}

	return result;
}

