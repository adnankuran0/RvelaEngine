#include "rvelapch.h"
#include "ParticleEmitterComponent.h"

using namespace rv;

json ParticleEmitterComponent::Serialize() const
{
	json j;
	j["emitting"] = emitting;
	j["amount"] = amount;

	// Time
	j["lifetime"] = lifetime;
	j["oneShot"] = oneShot;
	j["speedScale"] = speedScale;
	j["explosiveness"] = explosiveness;
	j["randomness"] = randomness;
	j["lifetimeRandomness"] = lifetimeRandomness;

	// Drawing
	j["localCoords"] = localCoords;

	// Emission shape
	j["emitterShape"] = static_cast<int>(emitterShape);
	j["shapeDimensions"] = { shapeDimensions.x, shapeDimensions.y, shapeDimensions.z };

	// Direction
	j["direction"] = { direction.x, direction.y, direction.z };
	j["spread"] = spread;
	j["gravity"] = { gravity.x, gravity.y, gravity.z };

	// Initial velocity
	j["linearVelocityMin"] = linearVelocityMin;
	j["linearVelocityMax"] = linearVelocityMax;
	j["angularVelocityMin"] = angularVelocityMin;
	j["angularVelocityMax"] = angularVelocityMax;
	j["rotationMin"] = rotationMin;
	j["rotationMax"] = rotationMax;

	// Acceleration
	j["linearAccelMin"] = linearAccelMin;
	j["linearAccelMax"] = linearAccelMax;

	// Damping
	j["dampingMin"] = dampingMin;
	j["dampingMax"] = dampingMax;

	// Scale
	j["scaleMin"] = scaleMin;
	j["scaleMax"] = scaleMax;
	j["scaleEnd"] = scaleEnd;

	// Color
	j["startColor"] = { startColor.r, startColor.g, startColor.b, startColor.a };
	j["endColor"] = { endColor.r, endColor.g, endColor.b, endColor.a };

	return j;
}

void ParticleEmitterComponent::Deserialize(const json& j)
{
	emitting = j.value("emitting", true);
	amount = j.value("amount", 8u);

	// Time
	lifetime = j.value("lifetime", 1.0f);
	oneShot = j.value("oneShot", false);
	speedScale = j.value("speedScale", 1.0f);
	explosiveness = j.value("explosiveness", 0.0f);
	randomness = j.value("randomness", 0.0f);
	lifetimeRandomness = j.value("lifetimeRandomness", 0.0f);

	// Drawing
	localCoords = j.value("localCoords", false);

	// Emission shape
	emitterShape = static_cast<ParticleEmitterShape>(j.value("emitterShape", 0));
	if (j.contains("shapeDimensions") && j["shapeDimensions"].is_array() && j["shapeDimensions"].size() >= 3)
	{
		const auto& d = j["shapeDimensions"];
		shapeDimensions = glm::vec3(d[0], d[1], d[2]);
	}

	// Direction
	if (j.contains("direction") && j["direction"].is_array() && j["direction"].size() >= 3)
	{
		const auto& dir = j["direction"];
		direction = glm::vec3(dir[0], dir[1], dir[2]);
	}
	spread = j.value("spread", 45.0f);
	if (j.contains("gravity") && j["gravity"].is_array() && j["gravity"].size() >= 3)
	{
		const auto& g = j["gravity"];
		gravity = glm::vec3(g[0], g[1], g[2]);
	}

	// Initial velocity
	linearVelocityMin = j.value("linearVelocityMin", 0.0f);
	linearVelocityMax = j.value("linearVelocityMax", 0.0f);
	angularVelocityMin = j.value("angularVelocityMin", 0.0f);
	angularVelocityMax = j.value("angularVelocityMax", 0.0f);
	rotationMin = j.value("rotationMin", 0.0f);
	rotationMax = j.value("rotationMax", 0.0f);

	// Acceleration
	linearAccelMin = j.value("linearAccelMin", 0.0f);
	linearAccelMax = j.value("linearAccelMax", 0.0f);

	// Damping
	dampingMin = j.value("dampingMin", 0.0f);
	dampingMax = j.value("dampingMax", 0.0f);

	// Scale
	scaleMin = j.value("scaleMin", 1.0f);
	scaleMax = j.value("scaleMax", 1.0f);
	scaleEnd = j.value("scaleEnd", 1.0f);

	// Color
	if (j.contains("startColor") && j["startColor"].is_array() && j["startColor"].size() >= 4)
	{
		const auto& c = j["startColor"];
		startColor = glm::vec4(c[0], c[1], c[2], c[3]);
	}
	if (j.contains("endColor") && j["endColor"].is_array() && j["endColor"].size() >= 4)
	{
		const auto& c = j["endColor"];
		endColor = glm::vec4(c[0], c[1], c[2], c[3]);
	}

	// Reset runtime
	timeElapsed = 0.0f;
	emissionTimer = 0.0f;
	cycleSpawnCount = 0;
	isFinished = false;
}