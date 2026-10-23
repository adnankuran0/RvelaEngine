#include "rvelapch.h"
#include "PhysicsWorld.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include "Scene/Components/PhysicsConstraintComponent.h"
#include "Scene/Components/UUIDComponent.h"
#include "Math/RvelaMath.h"
#include "UserData.h"

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>

#include <algorithm>
#include <cmath>

using namespace rv::Physics;

// Rigidbody API

void PhysicsWorld::SetPosition(RigidbodyComponent* comp, const glm::vec3& position)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetPosition(comp->RuntimeBodyID, math::ToJoltVec3(position), JPH::EActivation::Activate);
	comp->previousPosition = position;
	comp->currentPosition = position;
}

void PhysicsWorld::SetRotation(RigidbodyComponent* comp, const glm::quat& rotation)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetRotation(comp->RuntimeBodyID, math::ToJoltQuat(rotation), JPH::EActivation::Activate);
	comp->previousRotation = rotation;
	comp->currentRotation = rotation;
}

void PhysicsWorld::MoveKinematic(RigidbodyComponent* comp, 
	const glm::vec3& targetPosition, const glm::quat& targetRotation, float dt)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	JPH::EMotionType type = BodyInterface().GetMotionType(comp->RuntimeBodyID);
	if (type != JPH::EMotionType::Kinematic) return;

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
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return glm::vec3(0.0f);

	return math::FromJoltVec3(BodyInterface().GetLinearVelocity(comp->RuntimeBodyID));
}

void PhysicsWorld::SetMaxLinearVelocity(RigidbodyComponent* comp, float maxLinearVelocity)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetMaxLinearVelocity(comp->RuntimeBodyID, maxLinearVelocity);
}

float PhysicsWorld::GetMaxLinearVelocity(RigidbodyComponent* comp)
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
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return glm::vec3(0.0f);

	return math::FromJoltVec3(BodyInterface().GetAngularVelocity(comp->RuntimeBodyID));
}

void PhysicsWorld::SetMaxAngularVelocity(RigidbodyComponent* comp, float maxAngularVelocityInRadians)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetMaxAngularVelocity(comp->RuntimeBodyID, maxAngularVelocityInRadians);
}

float PhysicsWorld::GetMaxAngularVelocity(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return 0.0f;

	return BodyInterface().GetMaxAngularVelocity(comp->RuntimeBodyID);
}

glm::vec3 PhysicsWorld::GetCenterOfMassPosition(RigidbodyComponent* comp)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return glm::vec3(0.0f);

	return math::FromJoltVec3(BodyInterface().GetCenterOfMassPosition(comp->RuntimeBodyID));
}

void PhysicsWorld::SetFriction(RigidbodyComponent* comp, float friction)
{
	if (!comp || comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetFriction(comp->RuntimeBodyID, friction);
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

float PhysicsWorld::GetGravityFactor(RigidbodyComponent* comp)
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
	comp->previousPosition = position;
	comp->currentPosition = position;
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
	comp->previousRotation = rotation;
	comp->currentRotation = rotation;
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

// Advanced Raycast API

RaycastResult PhysicsWorld::Raycast(const glm::vec3& rayOrigin,
	const glm::vec3& rayDirection,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	RaycastOptions options;
	options.origin = rayOrigin;
	options.direction = rayDirection;
	options.maxDistance = maxDistance;
	options.hitInside = hitInside;
	options.ignoredEntities = ignoredEntities;
	return Raycast(options);
}

RaycastResult PhysicsWorld::Raycast(const RaycastOptions& options)
{
	RaycastResult result;
	if (!m_PhysicsSystem || options.maxDistance <= 0.0f) return result;

	float len = glm::length(options.direction);
	if (len < 0.0001f) return result;
	glm::vec3 dir = options.direction / len;

	JPH::RRayCast ray(
		math::ToJoltRVec3(options.origin),
		math::ToJoltVec3(dir * options.maxDistance)
	);

	JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
	JPH::RayCastSettings settings;
	settings.mTreatConvexAsSolid = options.hitInside;

	m_PhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, settings, collector);

	if (!collector.HadHit())
		return result;

	const JPH::RayCastResult* bestHit = nullptr;
	float bestFraction = FLT_MAX;

	for (const auto& hit : collector.mHits)
	{
		if (!options.hitInside && hit.mFraction <= 0.0001f) continue;

		if (!options.ignoredEntities.empty())
		{
			const JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), hit.mBodyID);
			if (lock.Succeeded())
			{
				const JPH::Body& body = lock.GetBody();
				auto* userData = reinterpret_cast<UserData*>(body.GetUserData());
				if (userData && std::find(options.ignoredEntities.begin(), options.ignoredEntities.end(), userData->entity) != options.ignoredEntities.end())
				{
					continue;
				}
			}
		}

		if (hit.mFraction < bestFraction)
		{
			bestFraction = hit.mFraction;
			bestHit = &hit;
		}
	}

	if (!bestHit)
		return result;

	float distance = bestHit->mFraction * options.maxDistance;
	JPH::RVec3 hitPoint = ray.mOrigin + ray.mDirection * bestHit->mFraction;

	const JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), bestHit->mBodyID);
	if (lock.Succeeded())
	{
		const JPH::Body& body = lock.GetBody();
		JPH::Vec3 normal = body.GetWorldSpaceSurfaceNormal(bestHit->mSubShapeID2, hitPoint);
		result.hit = true;
		result.distance = distance;
		result.fraction = bestHit->mFraction;
		result.point = math::FromJoltRVec3(hitPoint);
		result.normal = math::FromJoltVec3(normal);
		result.bodyId = bestHit->mBodyID.GetIndexAndSequenceNumber();
		auto* ud = reinterpret_cast<UserData*>(body.GetUserData());
		result.entity = ud ? ud->entity : entt::null;
	}

	return result;
}

std::vector<RaycastResult> PhysicsWorld::RaycastAll(const glm::vec3& rayOrigin,
	const glm::vec3& rayDirection,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	RaycastOptions options;
	options.origin = rayOrigin;
	options.direction = rayDirection;
	options.maxDistance = maxDistance;
	options.hitInside = hitInside;
	options.ignoredEntities = ignoredEntities;
	return RaycastAll(options);
}

std::vector<RaycastResult> PhysicsWorld::RaycastAll(const RaycastOptions& options)
{
	std::vector<RaycastResult> results;
	if (!m_PhysicsSystem || options.maxDistance <= 0.0f) return results;

	float len = glm::length(options.direction);
	if (len < 0.0001f) return results;
	glm::vec3 dir = options.direction / len;

	JPH::RRayCast ray(
		math::ToJoltRVec3(options.origin),
		math::ToJoltVec3(dir * options.maxDistance)
	);

	JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
	JPH::RayCastSettings settings;
	settings.mTreatConvexAsSolid = options.hitInside;

	m_PhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, settings, collector);

	if (!collector.HadHit())
		return results;

	// Sort hits by fraction ascending
	auto& hits = collector.mHits;
	std::sort(hits.begin(), hits.end(), [](const JPH::RayCastResult& a, const JPH::RayCastResult& b) {
		return a.mFraction < b.mFraction;
	});

	for (const auto& hit : hits)
	{
		if (!options.hitInside && hit.mFraction <= 0.0001f) continue;

		const JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), hit.mBodyID);
		if (!lock.Succeeded()) continue;

		const JPH::Body& body = lock.GetBody();
		auto* userData = reinterpret_cast<UserData*>(body.GetUserData());
		if (!options.ignoredEntities.empty() && userData)
		{
			if (std::find(options.ignoredEntities.begin(), options.ignoredEntities.end(), userData->entity) != options.ignoredEntities.end())
			{
				continue;
			}
		}

		RaycastResult res;
		res.hit = true;
		res.fraction = hit.mFraction;
		res.distance = hit.mFraction * options.maxDistance;
		JPH::RVec3 hitPoint = ray.mOrigin + ray.mDirection * hit.mFraction;
		res.point = math::FromJoltRVec3(hitPoint);
		JPH::Vec3 normal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, hitPoint);
		res.normal = math::FromJoltVec3(normal);
		res.entity = userData ? userData->entity : entt::null;
		res.bodyId = hit.mBodyID.GetIndexAndSequenceNumber();

		results.push_back(res);
	}

	return results;
}

// Shapecast API

ShapeCastResult PhysicsWorld::CastShape(const JPH::Shape* shape,
	const glm::vec3& start,
	const glm::quat& rotation,
	const glm::vec3& direction,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	ShapeCastResult result;
	if (!m_PhysicsSystem || !shape || maxDistance <= 0.0f) return result;

	float len = glm::length(direction);
	if (len < 0.0001f) return result;
	glm::vec3 dir = direction / len;

	JPH::RShapeCast shapeCast(
		shape,
		JPH::Vec3::sReplicate(1.0f),
		JPH::RMat44::sRotationTranslation(math::ToJoltQuat(rotation), math::ToJoltRVec3(start)),
		math::ToJoltVec3(dir * maxDistance)
	);

	JPH::ShapeCastSettings settings;
	settings.mBackFaceModeTriangles = hitInside ? JPH::EBackFaceMode::CollideWithBackFaces : JPH::EBackFaceMode::IgnoreBackFaces;
	settings.mBackFaceModeConvex = hitInside ? JPH::EBackFaceMode::CollideWithBackFaces : JPH::EBackFaceMode::IgnoreBackFaces;
	settings.mUseShrunkenShapeAndConvexRadius = false;
	settings.mReturnDeepestPoint = false;

	JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
	m_PhysicsSystem->GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

	if (!collector.HadHit())
		return result;

	const JPH::ShapeCastResult* bestHit = nullptr;
	float bestFraction = FLT_MAX;

	for (const auto& hit : collector.mHits)
	{
		if (!hitInside && hit.mFraction <= 0.0001f) continue;

		if (!ignoredEntities.empty())
		{
			const JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), hit.mBodyID2);
			if (lock.Succeeded())
			{
				const JPH::Body& body = lock.GetBody();
				auto* userData = reinterpret_cast<UserData*>(body.GetUserData());
				if (userData && std::find(ignoredEntities.begin(), ignoredEntities.end(), userData->entity) != ignoredEntities.end())
				{
					continue;
				}
			}
		}

		if (hit.mFraction < bestFraction)
		{
			bestFraction = hit.mFraction;
			bestHit = &hit;
		}
	}

	if (!bestHit)
		return result;

	result.hit = true;
	result.fraction = bestHit->mFraction;
	result.distance = bestHit->mFraction * maxDistance;
	result.point = math::FromJoltVec3(bestHit->mContactPointOn2);
	result.bodyId = bestHit->mBodyID2.GetIndexAndSequenceNumber();

	const JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), bestHit->mBodyID2);
	if (lock.Succeeded())
	{
		const JPH::Body& body = lock.GetBody();
		JPH::Vec3 normal = body.GetWorldSpaceSurfaceNormal(bestHit->mSubShapeID2, math::ToJoltRVec3(result.point));
		result.normal = math::FromJoltVec3(normal);
		auto* ud = reinterpret_cast<UserData*>(body.GetUserData());
		result.entity = ud ? ud->entity : entt::null;
	}

	if (glm::dot(result.normal, result.normal) < 0.0001f)
	{
		result.normal = math::FromJoltVec3(-bestHit->mPenetrationAxis.NormalizedOr(JPH::Vec3::sAxisY()));
	}

	return result;
}

std::vector<ShapeCastResult> PhysicsWorld::CastShapeAll(const JPH::Shape* shape,
	const glm::vec3& start,
	const glm::quat& rotation,
	const glm::vec3& direction,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	std::vector<ShapeCastResult> results;
	if (!m_PhysicsSystem || !shape || maxDistance <= 0.0f) return results;

	float len = glm::length(direction);
	if (len < 0.0001f) return results;
	glm::vec3 dir = direction / len;

	JPH::RShapeCast shapeCast(
		shape,
		JPH::Vec3::sReplicate(1.0f),
		JPH::RMat44::sRotationTranslation(math::ToJoltQuat(rotation), math::ToJoltRVec3(start)),
		math::ToJoltVec3(dir * maxDistance)
	);

	JPH::ShapeCastSettings settings;
	settings.mBackFaceModeTriangles = hitInside ? JPH::EBackFaceMode::CollideWithBackFaces : JPH::EBackFaceMode::IgnoreBackFaces;
	settings.mBackFaceModeConvex = hitInside ? JPH::EBackFaceMode::CollideWithBackFaces : JPH::EBackFaceMode::IgnoreBackFaces;
	settings.mUseShrunkenShapeAndConvexRadius = false;
	settings.mReturnDeepestPoint = false;

	JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
	m_PhysicsSystem->GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

	if (!collector.HadHit())
		return results;

	auto& hits = collector.mHits;
	std::sort(hits.begin(), hits.end(), [](const JPH::ShapeCastResult& a, const JPH::ShapeCastResult& b) {
		return a.mFraction < b.mFraction;
	});

	for (const auto& hit : hits)
	{
		if (!hitInside && hit.mFraction <= 0.0001f) continue;

		const JPH::BodyLockRead lock(m_PhysicsSystem->GetBodyLockInterface(), hit.mBodyID2);
		if (!lock.Succeeded()) continue;

		const JPH::Body& body = lock.GetBody();
		auto* userData = reinterpret_cast<UserData*>(body.GetUserData());
		if (!ignoredEntities.empty() && userData)
		{
			if (std::find(ignoredEntities.begin(), ignoredEntities.end(), userData->entity) != ignoredEntities.end())
			{
				continue;
			}
		}

		ShapeCastResult res;
		res.hit = true;
		res.fraction = hit.mFraction;
		res.distance = hit.mFraction * maxDistance;
		res.point = math::FromJoltVec3(hit.mContactPointOn2);
		res.bodyId = hit.mBodyID2.GetIndexAndSequenceNumber();
		res.entity = userData ? userData->entity : entt::null;

		JPH::Vec3 normal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, math::ToJoltRVec3(res.point));
		res.normal = math::FromJoltVec3(normal);
		if (glm::dot(res.normal, res.normal) < 0.0001f)
		{
			res.normal = math::FromJoltVec3(-hit.mPenetrationAxis.NormalizedOr(JPH::Vec3::sAxisY()));
		}

		results.push_back(res);
	}

	return results;
}

ShapeCastResult PhysicsWorld::SphereCast(const glm::vec3& start,
	float radius,
	const glm::vec3& direction,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	JPH::SphereShape shape(std::max(0.001f, radius));
	return CastShape(&shape, start, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), direction, maxDistance, hitInside, ignoredEntities);
}

std::vector<ShapeCastResult> PhysicsWorld::SphereCastAll(const glm::vec3& start,
	float radius,
	const glm::vec3& direction,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	JPH::SphereShape shape(std::max(0.001f, radius));
	return CastShapeAll(&shape, start, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), direction, maxDistance, hitInside, ignoredEntities);
}

ShapeCastResult PhysicsWorld::BoxCast(const glm::vec3& start,
	const glm::vec3& halfExtent,
	const glm::quat& rotation,
	const glm::vec3& direction,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	glm::vec3 ext = glm::max(halfExtent, glm::vec3(0.001f));
	JPH::BoxShape shape(math::ToJoltVec3(ext));
	return CastShape(&shape, start, rotation, direction, maxDistance, hitInside, ignoredEntities);
}

std::vector<ShapeCastResult> PhysicsWorld::BoxCastAll(const glm::vec3& start,
	const glm::vec3& halfExtent,
	const glm::quat& rotation,
	const glm::vec3& direction,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	glm::vec3 ext = glm::max(halfExtent, glm::vec3(0.001f));
	JPH::BoxShape shape(math::ToJoltVec3(ext));
	return CastShapeAll(&shape, start, rotation, direction, maxDistance, hitInside, ignoredEntities);
}

ShapeCastResult PhysicsWorld::CapsuleCast(const glm::vec3& start,
	float radius,
	float halfHeight,
	const glm::quat& rotation,
	const glm::vec3& direction,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	JPH::CapsuleShape shape(std::max(0.001f, halfHeight), std::max(0.001f, radius));
	return CastShape(&shape, start, rotation, direction, maxDistance, hitInside, ignoredEntities);
}

std::vector<ShapeCastResult> PhysicsWorld::CapsuleCastAll(const glm::vec3& start,
	float radius,
	float halfHeight,
	const glm::quat& rotation,
	const glm::vec3& direction,
	float maxDistance,
	bool hitInside,
	const std::vector<entt::entity>& ignoredEntities)
{
	JPH::CapsuleShape shape(std::max(0.001f, halfHeight), std::max(0.001f, radius));
	return CastShapeAll(&shape, start, rotation, direction, maxDistance, hitInside, ignoredEntities);
}

// Dynamic Constraints API

static glm::vec3 ComputePerpendicular(const glm::vec3& v)
{
	glm::vec3 normalized = glm::dot(v, v) > 0.0001f ? glm::normalize(v) : glm::vec3(0.0f, 1.0f, 0.0f);
	if (std::abs(normalized.y) < 0.99f)
		return glm::normalize(glm::cross(normalized, glm::vec3(0.0f, 1.0f, 0.0f)));
	return glm::normalize(glm::cross(normalized, glm::vec3(1.0f, 0.0f, 0.0f)));
}

uint64_t PhysicsWorld::CreateFixedConstraint(RigidbodyComponent* a, RigidbodyComponent* b, bool autoDetect)
{
	if (!m_PhysicsSystem || !a || a->RuntimeBodyID.IsInvalid()) return 0;

	JPH::FixedConstraintSettings settings;
	settings.mSpace = JPH::EConstraintSpace::WorldSpace;
	settings.mAutoDetectPoint = autoDetect;

	JPH::Ref<JPH::TwoBodyConstraint> constraint;
	if (b && !b->RuntimeBodyID.IsInvalid())
	{
		if (a->RuntimeBodyID == b->RuntimeBodyID) return 0;
		JPH::BodyID bodyIDs[2] = { a->RuntimeBodyID, b->RuntimeBodyID };
		JPH::BodyLockMultiWrite lock(m_PhysicsSystem->GetBodyLockInterface(), bodyIDs, 2);
		JPH::Body* bA = lock.GetBody(0);
		JPH::Body* bB = lock.GetBody(1);
		if (!bA || !bB) return 0;
		constraint = settings.Create(*bA, *bB);
	}
	else
	{
		JPH::BodyLockWrite lock(m_PhysicsSystem->GetBodyLockInterface(), a->RuntimeBodyID);
		if (!lock.Succeeded()) return 0;
		constraint = settings.Create(lock.GetBody(), JPH::Body::sFixedToWorld);
	}

	if (!constraint) return 0;

	uint64_t id = m_NextConstraintId++;
	m_PhysicsSystem->AddConstraint(constraint.GetPtr());
	m_DynamicConstraints[id] = constraint;
	return id;
}

uint64_t PhysicsWorld::CreatePointConstraint(RigidbodyComponent* a, RigidbodyComponent* b, const glm::vec3& pivot, bool worldSpace)
{
	if (!m_PhysicsSystem || !a || a->RuntimeBodyID.IsInvalid()) return 0;

	JPH::PointConstraintSettings settings;
	settings.mSpace = worldSpace ? JPH::EConstraintSpace::WorldSpace : JPH::EConstraintSpace::LocalToBodyCOM;
	settings.mPoint1 = math::ToJoltRVec3(pivot);
	settings.mPoint2 = math::ToJoltRVec3(pivot);

	JPH::Ref<JPH::TwoBodyConstraint> constraint;
	if (b && !b->RuntimeBodyID.IsInvalid())
	{
		if (a->RuntimeBodyID == b->RuntimeBodyID) return 0;
		JPH::BodyID bodyIDs[2] = { a->RuntimeBodyID, b->RuntimeBodyID };
		JPH::BodyLockMultiWrite lock(m_PhysicsSystem->GetBodyLockInterface(), bodyIDs, 2);
		JPH::Body* bA = lock.GetBody(0);
		JPH::Body* bB = lock.GetBody(1);
		if (!bA || !bB) return 0;
		constraint = settings.Create(*bA, *bB);
	}
	else
	{
		JPH::BodyLockWrite lock(m_PhysicsSystem->GetBodyLockInterface(), a->RuntimeBodyID);
		if (!lock.Succeeded()) return 0;
		constraint = settings.Create(lock.GetBody(), JPH::Body::sFixedToWorld);
	}

	if (!constraint) return 0;

	uint64_t id = m_NextConstraintId++;
	m_PhysicsSystem->AddConstraint(constraint.GetPtr());
	m_DynamicConstraints[id] = constraint;
	return id;
}

uint64_t PhysicsWorld::CreateDistanceConstraint(RigidbodyComponent* a, RigidbodyComponent* b,
	const glm::vec3& pointA, const glm::vec3& pointB, float minDistance, float maxDistance, bool worldSpace)
{
	if (!m_PhysicsSystem || !a || a->RuntimeBodyID.IsInvalid()) return 0;

	JPH::DistanceConstraintSettings settings;
	settings.mSpace = worldSpace ? JPH::EConstraintSpace::WorldSpace : JPH::EConstraintSpace::LocalToBodyCOM;
	settings.mPoint1 = math::ToJoltRVec3(pointA);
	settings.mPoint2 = math::ToJoltRVec3(pointB);
	settings.mMinDistance = minDistance;
	settings.mMaxDistance = maxDistance;

	JPH::Ref<JPH::TwoBodyConstraint> constraint;
	if (b && !b->RuntimeBodyID.IsInvalid())
	{
		if (a->RuntimeBodyID == b->RuntimeBodyID) return 0;
		JPH::BodyID bodyIDs[2] = { a->RuntimeBodyID, b->RuntimeBodyID };
		JPH::BodyLockMultiWrite lock(m_PhysicsSystem->GetBodyLockInterface(), bodyIDs, 2);
		JPH::Body* bA = lock.GetBody(0);
		JPH::Body* bB = lock.GetBody(1);
		if (!bA || !bB) return 0;
		constraint = settings.Create(*bA, *bB);
	}
	else
	{
		JPH::BodyLockWrite lock(m_PhysicsSystem->GetBodyLockInterface(), a->RuntimeBodyID);
		if (!lock.Succeeded()) return 0;
		constraint = settings.Create(lock.GetBody(), JPH::Body::sFixedToWorld);
	}

	if (!constraint) return 0;

	uint64_t id = m_NextConstraintId++;
	m_PhysicsSystem->AddConstraint(constraint.GetPtr());
	m_DynamicConstraints[id] = constraint;
	return id;
}

uint64_t PhysicsWorld::CreateHingeConstraint(RigidbodyComponent* a, RigidbodyComponent* b,
	const glm::vec3& pivot, const glm::vec3& axis, float minAngle, float maxAngle, bool worldSpace)
{
	if (!m_PhysicsSystem || !a || a->RuntimeBodyID.IsInvalid()) return 0;

	glm::vec3 norm = ComputePerpendicular(axis);

	JPH::HingeConstraintSettings settings;
	settings.mSpace = worldSpace ? JPH::EConstraintSpace::WorldSpace : JPH::EConstraintSpace::LocalToBodyCOM;
	settings.mPoint1 = math::ToJoltRVec3(pivot);
	settings.mPoint2 = math::ToJoltRVec3(pivot);
	settings.mHingeAxis1 = math::ToJoltVec3(axis);
	settings.mHingeAxis2 = math::ToJoltVec3(axis);
	settings.mNormalAxis1 = math::ToJoltVec3(norm);
	settings.mNormalAxis2 = math::ToJoltVec3(norm);
	settings.mLimitsMin = minAngle;
	settings.mLimitsMax = maxAngle;

	JPH::Ref<JPH::TwoBodyConstraint> constraint;
	if (b && !b->RuntimeBodyID.IsInvalid())
	{
		if (a->RuntimeBodyID == b->RuntimeBodyID) return 0;
		JPH::BodyID bodyIDs[2] = { a->RuntimeBodyID, b->RuntimeBodyID };
		JPH::BodyLockMultiWrite lock(m_PhysicsSystem->GetBodyLockInterface(), bodyIDs, 2);
		JPH::Body* bA = lock.GetBody(0);
		JPH::Body* bB = lock.GetBody(1);
		if (!bA || !bB) return 0;
		constraint = settings.Create(*bA, *bB);
	}
	else
	{
		JPH::BodyLockWrite lock(m_PhysicsSystem->GetBodyLockInterface(), a->RuntimeBodyID);
		if (!lock.Succeeded()) return 0;
		constraint = settings.Create(lock.GetBody(), JPH::Body::sFixedToWorld);
	}

	if (!constraint) return 0;

	uint64_t id = m_NextConstraintId++;
	m_PhysicsSystem->AddConstraint(constraint.GetPtr());
	m_DynamicConstraints[id] = constraint;
	return id;
}

uint64_t PhysicsWorld::CreateSliderConstraint(RigidbodyComponent* a, RigidbodyComponent* b,
	const glm::vec3& sliderAxis, float minLimit, float maxLimit, bool worldSpace)
{
	if (!m_PhysicsSystem || !a || a->RuntimeBodyID.IsInvalid()) return 0;

	glm::vec3 norm = ComputePerpendicular(sliderAxis);

	JPH::SliderConstraintSettings settings;
	settings.mSpace = worldSpace ? JPH::EConstraintSpace::WorldSpace : JPH::EConstraintSpace::LocalToBodyCOM;
	settings.mAutoDetectPoint = true;
	settings.mSliderAxis1 = math::ToJoltVec3(sliderAxis);
	settings.mSliderAxis2 = math::ToJoltVec3(sliderAxis);
	settings.mNormalAxis1 = math::ToJoltVec3(norm);
	settings.mNormalAxis2 = math::ToJoltVec3(norm);
	settings.mLimitsMin = minLimit;
	settings.mLimitsMax = maxLimit;

	JPH::Ref<JPH::TwoBodyConstraint> constraint;
	if (b && !b->RuntimeBodyID.IsInvalid())
	{
		if (a->RuntimeBodyID == b->RuntimeBodyID) return 0;
		JPH::BodyID bodyIDs[2] = { a->RuntimeBodyID, b->RuntimeBodyID };
		JPH::BodyLockMultiWrite lock(m_PhysicsSystem->GetBodyLockInterface(), bodyIDs, 2);
		JPH::Body* bA = lock.GetBody(0);
		JPH::Body* bB = lock.GetBody(1);
		if (!bA || !bB) return 0;
		constraint = settings.Create(*bA, *bB);
	}
	else
	{
		JPH::BodyLockWrite lock(m_PhysicsSystem->GetBodyLockInterface(), a->RuntimeBodyID);
		if (!lock.Succeeded()) return 0;
		constraint = settings.Create(lock.GetBody(), JPH::Body::sFixedToWorld);
	}

	if (!constraint) return 0;

	uint64_t id = m_NextConstraintId++;
	m_PhysicsSystem->AddConstraint(constraint.GetPtr());
	m_DynamicConstraints[id] = constraint;
	return id;
}

uint64_t PhysicsWorld::CreateConeConstraint(RigidbodyComponent* a, RigidbodyComponent* b,
	const glm::vec3& pivot, const glm::vec3& axis, float halfAngle, bool worldSpace)
{
	if (!m_PhysicsSystem || !a || a->RuntimeBodyID.IsInvalid()) return 0;

	JPH::ConeConstraintSettings settings;
	settings.mSpace = worldSpace ? JPH::EConstraintSpace::WorldSpace : JPH::EConstraintSpace::LocalToBodyCOM;
	settings.mPoint1 = math::ToJoltRVec3(pivot);
	settings.mPoint2 = math::ToJoltRVec3(pivot);
	settings.mTwistAxis1 = math::ToJoltVec3(axis);
	settings.mTwistAxis2 = math::ToJoltVec3(axis);
	settings.mHalfConeAngle = halfAngle;

	JPH::Ref<JPH::TwoBodyConstraint> constraint;
	if (b && !b->RuntimeBodyID.IsInvalid())
	{
		if (a->RuntimeBodyID == b->RuntimeBodyID) return 0;
		JPH::BodyID bodyIDs[2] = { a->RuntimeBodyID, b->RuntimeBodyID };
		JPH::BodyLockMultiWrite lock(m_PhysicsSystem->GetBodyLockInterface(), bodyIDs, 2);
		JPH::Body* bA = lock.GetBody(0);
		JPH::Body* bB = lock.GetBody(1);
		if (!bA || !bB) return 0;
		constraint = settings.Create(*bA, *bB);
	}
	else
	{
		JPH::BodyLockWrite lock(m_PhysicsSystem->GetBodyLockInterface(), a->RuntimeBodyID);
		if (!lock.Succeeded()) return 0;
		constraint = settings.Create(lock.GetBody(), JPH::Body::sFixedToWorld);
	}

	if (!constraint) return 0;

	uint64_t id = m_NextConstraintId++;
	m_PhysicsSystem->AddConstraint(constraint.GetPtr());
	m_DynamicConstraints[id] = constraint;
	return id;
}

bool PhysicsWorld::DestroyConstraint(uint64_t constraintId)
{
	if (!m_PhysicsSystem) return false;
	auto it = m_DynamicConstraints.find(constraintId);
	if (it == m_DynamicConstraints.end()) return false;

	m_PhysicsSystem->RemoveConstraint(it->second.GetPtr());
	m_DynamicConstraints.erase(it);
	return true;
}

bool PhysicsWorld::SetConstraintEnabled(uint64_t constraintId, bool enabled)
{
	auto it = m_DynamicConstraints.find(constraintId);
	if (it == m_DynamicConstraints.end() || !it->second) return false;

	it->second->SetEnabled(enabled);
	return true;
}

bool PhysicsWorld::IsConstraintEnabled(uint64_t constraintId)
{
	auto it = m_DynamicConstraints.find(constraintId);
	if (it == m_DynamicConstraints.end() || !it->second) return false;

	return it->second->GetEnabled();
}

void PhysicsWorld::SetHingeLimits(uint64_t constraintId, float minAngle, float maxAngle)
{
	auto it = m_DynamicConstraints.find(constraintId);
	if (it != m_DynamicConstraints.end() && it->second)
	{
		if (auto* hinge = dynamic_cast<JPH::HingeConstraint*>(it->second.GetPtr()))
		{
			hinge->SetLimits(minAngle, maxAngle);
		}
	}
}

void PhysicsWorld::SetHingeMotor(uint64_t constraintId, bool enabled, float targetVelocity, float maxTorque)
{
	auto it = m_DynamicConstraints.find(constraintId);
	if (it != m_DynamicConstraints.end() && it->second)
	{
		if (auto* hinge = dynamic_cast<JPH::HingeConstraint*>(it->second.GetPtr()))
		{
			float absTorque = std::abs(maxTorque);
			hinge->GetMotorSettings().mMinTorqueLimit = -absTorque;
			hinge->GetMotorSettings().mMaxTorqueLimit = absTorque;
			hinge->SetTargetAngularVelocity(targetVelocity);
			hinge->SetMotorState(enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
		}
	}
}

float PhysicsWorld::GetHingeAngle(uint64_t constraintId)
{
	auto it = m_DynamicConstraints.find(constraintId);
	if (it != m_DynamicConstraints.end() && it->second)
	{
		if (auto* hinge = dynamic_cast<JPH::HingeConstraint*>(it->second.GetPtr()))
		{
			return hinge->GetCurrentAngle();
		}
	}
	return 0.0f;
}

void PhysicsWorld::SetSliderLimits(uint64_t constraintId, float minLimit, float maxLimit)
{
	auto it = m_DynamicConstraints.find(constraintId);
	if (it != m_DynamicConstraints.end() && it->second)
	{
		if (auto* slider = dynamic_cast<JPH::SliderConstraint*>(it->second.GetPtr()))
		{
			slider->SetLimits(minLimit, maxLimit);
		}
	}
}

void PhysicsWorld::SetSliderMotor(uint64_t constraintId, bool enabled, float targetVelocity, float maxForce)
{
	auto it = m_DynamicConstraints.find(constraintId);
	if (it != m_DynamicConstraints.end() && it->second)
	{
		if (auto* slider = dynamic_cast<JPH::SliderConstraint*>(it->second.GetPtr()))
		{
			float absForce = std::abs(maxForce);
			slider->GetMotorSettings().mMinForceLimit = -absForce;
			slider->GetMotorSettings().mMaxForceLimit = absForce;
			slider->SetTargetVelocity(targetVelocity);
			slider->SetMotorState(enabled ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
		}
	}
}

float PhysicsWorld::GetSliderPosition(uint64_t constraintId)
{
	auto it = m_DynamicConstraints.find(constraintId);
	if (it != m_DynamicConstraints.end() && it->second)
	{
		if (auto* slider = dynamic_cast<JPH::SliderConstraint*>(it->second.GetPtr()))
		{
			return slider->GetCurrentPosition();
		}
	}
	return 0.0f;
}

void PhysicsWorld::SetDistanceRange(uint64_t constraintId, float minDistance, float maxDistance)
{
	auto it = m_DynamicConstraints.find(constraintId);
	if (it != m_DynamicConstraints.end() && it->second)
	{
		if (auto* dist = dynamic_cast<JPH::DistanceConstraint*>(it->second.GetPtr()))
		{
			dist->SetDistance(minDistance, maxDistance);
		}
	}
}

void PhysicsWorld::DestroyAllDynamicConstraints()
{
	if (m_PhysicsSystem)
	{
		for (auto& [id, c] : m_DynamicConstraints)
		{
			if (c)
				m_PhysicsSystem->RemoveConstraint(c.GetPtr());
		}
	}
	m_DynamicConstraints.clear();
}

// Component constraints helpers

bool PhysicsWorld::BuildConstraintComponent(entt::registry& reg, entt::entity e, PhysicsConstraintComponent& comp)
{
	if (!m_PhysicsSystem) return false;
	auto* rbA = reg.try_get<RigidbodyComponent>(e);
	if (!rbA || rbA->RuntimeBodyID.IsInvalid()) return false;

	JPH::BodyID bodyA_ID = rbA->RuntimeBodyID;
	JPH::BodyID bodyB_ID = JPH::BodyID();

	if (comp.connectedEntityUUID != 0)
	{
		entt::entity targetEntity = entt::null;
		auto uuidView = reg.view<UUIDComponent>();
		for (auto other : uuidView)
		{
			if (uuidView.get<UUIDComponent>(other).uuid == comp.connectedEntityUUID)
			{
				targetEntity = other;
				break;
			}
		}

		if (targetEntity == entt::null || !reg.valid(targetEntity))
			return false;

		auto* rbB = reg.try_get<RigidbodyComponent>(targetEntity);
		if (!rbB || rbB->RuntimeBodyID.IsInvalid())
			return false;

		bodyB_ID = rbB->RuntimeBodyID;
	}

	if (comp.runtimeConstraint)
	{
		m_PhysicsSystem->RemoveConstraint(comp.runtimeConstraint.GetPtr());
		comp.runtimeConstraint = nullptr;
	}

	JPH::Ref<JPH::TwoBodyConstraint> constraint;
	JPH::EConstraintSpace space = comp.space == ConstraintSpace::LocalSpace ? JPH::EConstraintSpace::LocalToBodyCOM : JPH::EConstraintSpace::WorldSpace;

	auto CreateWithSettings = [&](const JPH::TwoBodyConstraintSettings& settings) {
		if (bodyB_ID.IsInvalid())
		{
			JPH::BodyLockWrite lockA(m_PhysicsSystem->GetBodyLockInterface(), bodyA_ID);
			if (lockA.Succeeded())
			{
				constraint = settings.Create(lockA.GetBody(), JPH::Body::sFixedToWorld);
			}
		}
		else
		{
			if (bodyA_ID == bodyB_ID) return;
			JPH::BodyID bodyIDs[2] = { bodyA_ID, bodyB_ID };
			JPH::BodyLockMultiWrite lock(m_PhysicsSystem->GetBodyLockInterface(), bodyIDs, 2);
			JPH::Body* bA = lock.GetBody(0);
			JPH::Body* bB = lock.GetBody(1);
			if (bA && bB)
			{
				constraint = settings.Create(*bA, *bB);
			}
		}
	};

	switch (comp.type)
	{
	case ConstraintType::Fixed:
	{
		JPH::FixedConstraintSettings settings;
		settings.mSpace = space;
		settings.mAutoDetectPoint = comp.autoDetectPoints;
		if (!comp.autoDetectPoints)
		{
			settings.mPoint1 = math::ToJoltRVec3(comp.anchorA);
			settings.mPoint2 = math::ToJoltRVec3(comp.anchorB);
		}
		CreateWithSettings(settings);
		break;
	}
	case ConstraintType::Point:
	{
		JPH::PointConstraintSettings settings;
		settings.mSpace = space;
		settings.mPoint1 = math::ToJoltRVec3(comp.anchorA);
		settings.mPoint2 = math::ToJoltRVec3(comp.anchorB);
		CreateWithSettings(settings);
		break;
	}
	case ConstraintType::Distance:
	{
		JPH::DistanceConstraintSettings settings;
		settings.mSpace = space;
		settings.mPoint1 = math::ToJoltRVec3(comp.anchorA);
		settings.mPoint2 = math::ToJoltRVec3(comp.anchorB);
		settings.mMinDistance = comp.minLimit;
		settings.mMaxDistance = comp.maxLimit;
		if (comp.springFrequency > 0.0f)
		{
			settings.mLimitsSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
			settings.mLimitsSpringSettings.mFrequency = comp.springFrequency;
			settings.mLimitsSpringSettings.mDamping = comp.springDamping;
		}
		CreateWithSettings(settings);
		break;
	}
	case ConstraintType::Hinge:
	{
		JPH::HingeConstraintSettings settings;
		settings.mSpace = space;
		settings.mPoint1 = math::ToJoltRVec3(comp.anchorA);
		settings.mPoint2 = math::ToJoltRVec3(comp.anchorB);
		settings.mHingeAxis1 = math::ToJoltVec3(comp.axisA);
		settings.mHingeAxis2 = math::ToJoltVec3(comp.axisB);
		settings.mNormalAxis1 = math::ToJoltVec3(comp.normalA);
		settings.mNormalAxis2 = math::ToJoltVec3(comp.normalB);
		settings.mLimitsMin = comp.minLimit;
		settings.mLimitsMax = comp.maxLimit;
		settings.mMaxFrictionTorque = comp.maxFriction;
		if (comp.motorEnabled)
		{
			float absTorque = std::abs(comp.motorMaxForce);
			settings.mMotorSettings.mMinTorqueLimit = -absTorque;
			settings.mMotorSettings.mMaxTorqueLimit = absTorque;
		}
		CreateWithSettings(settings);
		if (constraint && comp.motorEnabled)
		{
			auto* hinge = static_cast<JPH::HingeConstraint*>(constraint.GetPtr());
			hinge->SetTargetAngularVelocity(comp.motorTargetVelocity);
			hinge->SetMotorState(JPH::EMotorState::Velocity);
		}
		break;
	}
	case ConstraintType::Slider:
	{
		JPH::SliderConstraintSettings settings;
		settings.mSpace = space;
		settings.mAutoDetectPoint = comp.autoDetectPoints;
		settings.mPoint1 = math::ToJoltRVec3(comp.anchorA);
		settings.mPoint2 = math::ToJoltRVec3(comp.anchorB);
		settings.mSliderAxis1 = math::ToJoltVec3(comp.axisA);
		settings.mSliderAxis2 = math::ToJoltVec3(comp.axisB);
		settings.mNormalAxis1 = math::ToJoltVec3(comp.normalA);
		settings.mNormalAxis2 = math::ToJoltVec3(comp.normalB);
		settings.mLimitsMin = comp.minLimit;
		settings.mLimitsMax = comp.maxLimit;
		settings.mMaxFrictionForce = comp.maxFriction;
		if (comp.motorEnabled)
		{
			float absForce = std::abs(comp.motorMaxForce);
			settings.mMotorSettings.mMinForceLimit = -absForce;
			settings.mMotorSettings.mMaxForceLimit = absForce;
		}
		CreateWithSettings(settings);
		if (constraint && comp.motorEnabled)
		{
			auto* slider = static_cast<JPH::SliderConstraint*>(constraint.GetPtr());
			slider->SetTargetVelocity(comp.motorTargetVelocity);
			slider->SetMotorState(JPH::EMotorState::Velocity);
		}
		break;
	}
	case ConstraintType::Cone:
	{
		JPH::ConeConstraintSettings settings;
		settings.mSpace = space;
		settings.mPoint1 = math::ToJoltRVec3(comp.anchorA);
		settings.mPoint2 = math::ToJoltRVec3(comp.anchorB);
		settings.mTwistAxis1 = math::ToJoltVec3(comp.axisA);
		settings.mTwistAxis2 = math::ToJoltVec3(comp.axisB);
		settings.mHalfConeAngle = comp.maxLimit;
		CreateWithSettings(settings);
		break;
	}
	}

	if (!constraint) return false;

	constraint->SetEnabled(comp.enabled);
	m_PhysicsSystem->AddConstraint(constraint.GetPtr());
	comp.runtimeConstraint = constraint;
	comp.ClearDirty();
	return true;
}

void PhysicsWorld::RemoveConstraintComponent(PhysicsConstraintComponent& comp)
{
	if (m_PhysicsSystem && comp.runtimeConstraint)
	{
		m_PhysicsSystem->RemoveConstraint(comp.runtimeConstraint.GetPtr());
		comp.runtimeConstraint = nullptr;
	}
}
