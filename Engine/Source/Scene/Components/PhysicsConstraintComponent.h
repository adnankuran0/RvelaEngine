#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../nlohmann/json.hpp"
#include "Scene/EntityUUID.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>

namespace rv {

using json = nlohmann::json;

enum class ConstraintType
{
	Fixed = 0,
	Point,
	Distance,
	Hinge,
	Slider,
	Cone
};

enum class ConstraintSpace
{
	WorldSpace = 0,
	LocalSpace
};

struct PhysicsConstraintComponent
{
	EntityUUID connectedEntityUUID; // Empty  means static

	ConstraintType type = ConstraintType::Fixed;
	ConstraintSpace space = ConstraintSpace::WorldSpace;

	bool enabled = true;
	bool autoDetectPoints = true;

	glm::vec3 anchorA = glm::vec3(0.0f);
	glm::vec3 anchorB = glm::vec3(0.0f);

	glm::vec3 axisA = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 axisB = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 normalA = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 normalB = glm::vec3(1.0f, 0.0f, 0.0f);

	float minLimit = 0.0f;
	float maxLimit = 0.0f;

	float springFrequency = 0.0f;
	float springDamping = 0.0f;

	float maxFriction = 0.0f;

	bool motorEnabled = false;
	float motorTargetVelocity = 0.0f;
	float motorMaxForce = 0.0f;

	JPH::Ref<JPH::TwoBodyConstraint> runtimeConstraint;

	void SetDirty() { shapeDirty = true; }
	void ClearDirty() { shapeDirty = false; }
	bool IsDirty() const { return shapeDirty; }

	json Serialize() const;
	void Deserialize(const json& j);

private:
	bool shapeDirty = true;
};

}
