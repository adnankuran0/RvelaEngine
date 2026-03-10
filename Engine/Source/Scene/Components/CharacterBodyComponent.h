#pragma once
#include "../nlohmann/json.hpp"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Physics/CollisionFilter.h"

namespace rv {
	
using json = nlohmann::json;

struct CharacterBodyComponent
{
	JPH::Ref<JPH::CharacterVirtual> character;

	float mass = 70.0f;
	float maxStrength = 100.0f;
	glm::vec3 shapeOffset{ 0.0,0.0,0.0 };
	float predictiveContactDistance = 0.1f;
	float maxSlopeAngle = 50.0f; // degrees

	Physics::CollisionFilter collisionFilter;

	bool interpolationReady = false;
	glm::vec3 previousPosition{};
	glm::quat previousRotation{ 1,0,0,0 };
	glm::vec3 currentPosition{};
	glm::quat currentRotation{ 1,0,0,0 };

	json Serialize() const;
	void Deserialize(const json& j);
};

}