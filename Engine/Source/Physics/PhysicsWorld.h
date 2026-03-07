#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "MotionType.h"

namespace rv {
struct RigidbodyComponent;
struct CharacterBodyComponent;
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

	void SetMotionType(RigidbodyComponent* comp, MotionType motionType); // TODO: MotionType usertype for script api
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

	void SetIsSensor(RigidbodyComponent* comp,bool isSensor);
	bool IsSensor(RigidbodyComponent* comp);


	//CharacterBody API
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

private:
	inline JPH::BodyInterface& BodyInterface() { return m_PhysicsSystem->GetBodyInterface(); }
	JPH::PhysicsSystem* m_PhysicsSystem = nullptr;

};


}
