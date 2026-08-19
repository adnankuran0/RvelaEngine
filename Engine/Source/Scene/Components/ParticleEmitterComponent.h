#pragma once
#include "json.hpp"
#include "glm/glm.hpp"

namespace rv {

using json = nlohmann::json;

enum class ParticleEmitterShape : uint8_t
{
	Point = 0,
	Sphere = 1,
	SphereSurface = 2,
	Box = 3,
};

struct ParticleEmitterComponent
{
	bool emitting = true;
	uint32_t amount = 8;

	// Time
	float lifetime = 1.0f; // sec
	bool oneShot = false;
	float speedScale = 1.0f; 
	float explosiveness = 0.0f;
	float randomness = 0.0f;
	float lifetimeRandomness = 0.0f;

	// Drawing
	bool localCoords = false;

	// Emission shape
	ParticleEmitterShape emitterShape = ParticleEmitterShape::Point;
	glm::vec3 shapeDimensions = glm::vec3(1.0f);

	// Direction
	glm::vec3 direction = glm::vec3(1.0,0.0,0.0);
	float spread = 45.0f; // 0 - 180
	glm::vec3 gravity = glm::vec3(0.0,-9.8,0.0);

	// Initial velocity
	float linearVelocityMin = 0.0f;
	float linearVelocityMax = 0.0f;
	float angularVelocityMin = 0.0f;
	float angularVelocityMax = 0.0f;
	float rotationMin = 0.0f;
	float rotationMax = 0.0f;

	// Acceleration
	float linearAccelMin = 0.0f;
	float linearAccelMax = 0.0f;
	
	// Damping
	float dampingMin = 0.0f;
	float dampingMax = 0.0f;
	
	// Scale
	float scaleMin = 1.0f;
	float scaleMax = 1.0f;
	float scaleEnd = 1.0f;

	// Color
	glm::vec4 startColor = glm::vec4(1.0f);
	glm::vec4 endColor = glm::vec4(1.0f);

	// Runtime
	float timeElapsed = 0.0f;
	float emissionTimer = 0.0f;
	uint32_t cycleSpawnCount = 0;
	bool isFinished = false;

	json Serialize() const;
	void Deserialize(const json& j);

};

}