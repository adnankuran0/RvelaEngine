#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Constraints/TwoBodyConstraint.h"
#include "MotionType.h"
#include "RaycastResult.h"
#include <vector>
#include <unordered_map>

namespace rv {
struct RigidbodyComponent;
struct CharacterBodyComponent;
struct PhysicsConstraintComponent;
}

namespace rv::Physics {

class PhysicsWorld
{
public:
	void Init(JPH::PhysicsSystem& physicsSystem) { m_PhysicsSystem = &physicsSystem; }

	// Rigidbody API
	void SetPosition(RigidbodyComponent* comp, const glm::vec3& position);
	void SetRotation(RigidbodyComponent* comp, const glm::quat& rotation);

	void MoveKinematic(RigidbodyComponent* comp, const glm::vec3& targetPosition, 
		const glm::quat& targetRotation, float dt);

	void SetMotionType(RigidbodyComponent* comp, MotionType motionType);
	MotionType GetMotionType(RigidbodyComponent* comp);

	void AddForce(RigidbodyComponent* comp, const glm::vec3& force);
	void AddImpulse(RigidbodyComponent* comp, const glm::vec3& impulse);
	void AddTorque(RigidbodyComponent* comp, const glm::vec3& impulse);
	void AddAngularImpulse(RigidbodyComponent* comp, const glm::vec3& angularImpulse);

	void AddLinearVelocity(RigidbodyComponent* comp, const glm::vec3& linearVelocity);
	void SetLinearVelocity(RigidbodyComponent* comp, const glm::vec3& linearVelocity);
	glm::vec3 GetLinearVelocity(RigidbodyComponent* comp);
	void SetMaxLinearVelocity(RigidbodyComponent* comp, float maxLinearVelocity);
	float GetMaxLinearVelocity(RigidbodyComponent* comp);

	void SetAngularVelocity(RigidbodyComponent* comp, const glm::vec3& angularVelociry);
	glm::vec3 GetAngularVelocity(RigidbodyComponent* comp);
	void SetMaxAngularVelocity(RigidbodyComponent* comp, float maxAngularVelocityInRadians);
	float GetMaxAngularVelocity(RigidbodyComponent* comp);

	glm::vec3 GetCenterOfMassPosition(RigidbodyComponent* comp);
	
	void SetFriction(RigidbodyComponent* comp, float friction);
	float GetFriction(RigidbodyComponent* comp);

	void SetGravityFactor(RigidbodyComponent* comp, float gravityFactor);
	float GetGravityFactor(RigidbodyComponent* comp);

	void SetRestitution(RigidbodyComponent* comp, float restitution);
	float GetRestitution(RigidbodyComponent* comp);

	void SetIsSensor(RigidbodyComponent* comp, bool isSensor);
	bool IsSensor(RigidbodyComponent* comp);

	// CharacterBody API
	bool IsCharacterGrounded(CharacterBodyComponent* comp);
	bool IsCharacterOnSteepGround(CharacterBodyComponent* comp);
	bool IsCharacterInAir(CharacterBodyComponent* comp);
	bool IsCharacterNotSupported(CharacterBodyComponent* comp);

	glm::vec3 GetCharacterPosition(CharacterBodyComponent* comp);
	void SetCharacterPosition(CharacterBodyComponent* comp, const glm::vec3& position);
	glm::quat GetCharacterRotation(CharacterBodyComponent* comp);
	void SetCharacterRotation(CharacterBodyComponent* comp, const glm::quat& rotation);
	glm::vec3 GetCharacterCenterOfMassPosition(CharacterBodyComponent* comp);

	glm::vec3 GetCharacterLinearVelocity(CharacterBodyComponent* comp);
	void SetCharacterLinearVelocity(CharacterBodyComponent* comp, const glm::vec3& velocity);

	glm::vec3 GetCharacterUp(CharacterBodyComponent* comp);
	void SetCharacterUp(CharacterBodyComponent* comp, const glm::vec3& up);
	float GetCharacterMaxSlopeAngle(CharacterBodyComponent* comp);
	void SetCharacterMaxSlopeAngle(CharacterBodyComponent* comp, float angleInRadians);

	glm::vec3 GetCharacterGroundNormal(CharacterBodyComponent* comp);
	glm::vec3 GetCharacterGroundVelocity(CharacterBodyComponent* comp);
	glm::vec3 GetCharacterGroundPosition(CharacterBodyComponent* comp);

	RaycastResult Raycast(const glm::vec3& rayOrigin,
		const glm::vec3& rayDirection,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	RaycastResult Raycast(const RaycastOptions& options);

	std::vector<RaycastResult> RaycastAll(const glm::vec3& rayOrigin,
		const glm::vec3& rayDirection,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	std::vector<RaycastResult> RaycastAll(const RaycastOptions& options);

	ShapeCastResult SphereCast(const glm::vec3& start,
		float radius,
		const glm::vec3& direction,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	std::vector<ShapeCastResult> SphereCastAll(const glm::vec3& start,
		float radius,
		const glm::vec3& direction,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	ShapeCastResult BoxCast(const glm::vec3& start,
		const glm::vec3& halfExtent,
		const glm::quat& rotation,
		const glm::vec3& direction,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	std::vector<ShapeCastResult> BoxCastAll(const glm::vec3& start,
		const glm::vec3& halfExtent,
		const glm::quat& rotation,
		const glm::vec3& direction,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	ShapeCastResult CapsuleCast(const glm::vec3& start,
		float radius,
		float halfHeight,
		const glm::quat& rotation,
		const glm::vec3& direction,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	std::vector<ShapeCastResult> CapsuleCastAll(const glm::vec3& start,
		float radius,
		float halfHeight,
		const glm::quat& rotation,
		const glm::vec3& direction,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	ShapeCastResult CastShape(const JPH::Shape* shape,
		const glm::vec3& start,
		const glm::quat& rotation,
		const glm::vec3& direction,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	std::vector<ShapeCastResult> CastShapeAll(const JPH::Shape* shape,
		const glm::vec3& start,
		const glm::quat& rotation,
		const glm::vec3& direction,
		float maxDistance,
		bool hitInside = false,
		const std::vector<entt::entity>& ignoredEntities = {});

	uint64_t CreateFixedConstraint(RigidbodyComponent* a, RigidbodyComponent* b, bool autoDetect = true);
	uint64_t CreatePointConstraint(RigidbodyComponent* a, RigidbodyComponent* b, const glm::vec3& pivot, bool worldSpace = true);
	uint64_t CreateDistanceConstraint(RigidbodyComponent* a, RigidbodyComponent* b, const glm::vec3& pointA, const glm::vec3& pointB, float minDistance = -1.0f, float maxDistance = -1.0f, bool worldSpace = true);
	uint64_t CreateHingeConstraint(RigidbodyComponent* a, RigidbodyComponent* b, const glm::vec3& pivot, const glm::vec3& axis, float minAngle = -3.14159265f, float maxAngle = 3.14159265f, bool worldSpace = true);
	uint64_t CreateSliderConstraint(RigidbodyComponent* a, RigidbodyComponent* b, const glm::vec3& sliderAxis, float minLimit = -100.0f, float maxLimit = 100.0f, bool worldSpace = true);
	uint64_t CreateConeConstraint(RigidbodyComponent* a, RigidbodyComponent* b, const glm::vec3& pivot, const glm::vec3& axis, float halfAngle = 0.785398f, bool worldSpace = true);

	bool DestroyConstraint(uint64_t constraintId);
	bool SetConstraintEnabled(uint64_t constraintId, bool enabled);
	bool IsConstraintEnabled(uint64_t constraintId);

	void SetHingeLimits(uint64_t constraintId, float minAngle, float maxAngle);
	void SetHingeMotor(uint64_t constraintId, bool enabled, float targetVelocity, float maxTorque);
	float GetHingeAngle(uint64_t constraintId);

	void SetSliderLimits(uint64_t constraintId, float minLimit, float maxLimit);
	void SetSliderMotor(uint64_t constraintId, bool enabled, float targetVelocity, float maxForce);
	float GetSliderPosition(uint64_t constraintId);

	void SetDistanceRange(uint64_t constraintId, float minDistance, float maxDistance);

	void DestroyAllDynamicConstraints();

	bool BuildConstraintComponent(entt::registry& reg, entt::entity e, PhysicsConstraintComponent& comp);
	void RemoveConstraintComponent(PhysicsConstraintComponent& comp);

	inline JPH::BodyInterface& BodyInterface() { return m_PhysicsSystem->GetBodyInterface(); }

private:
	JPH::PhysicsSystem* m_PhysicsSystem = nullptr;

	uint64_t m_NextConstraintId = 1;
	std::unordered_map<uint64_t, JPH::Ref<JPH::TwoBodyConstraint>> m_DynamicConstraints;
};

}
