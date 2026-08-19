#include "rvelapch.h"
#include "ParticleSystem.h"
#include "Scene/Scene.h"
#include <glm/gtc/random.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Math/RvelaMath.h"

using namespace rv;

// TODO: move to rv::math
static float RandomFloat(float min, float max)
{
	if (min >= max) return min;
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void EmitterParticlePool::Resize(uint32_t newCapacity)
{
	capacity = newCapacity;
	activeCount = std::min(activeCount, capacity);

	position.resize(capacity);
	velocity.resize(capacity);
	acceleration.resize(capacity);
	damping.resize(capacity);
	currentLife.resize(capacity);
	maxLife.resize(capacity);
	startScale.resize(capacity);
	endScale.resize(capacity);
	rotation.resize(capacity);
	rotationSpeed.resize(capacity);
	startColor.resize(capacity);
	endColor.resize(capacity);
}

void EmitterParticlePool::SwapAndPop(uint32_t deadIndex)
{
	if (activeCount == 0 || deadIndex >= activeCount) return;

	uint32_t last = activeCount - 1;
	if (deadIndex != last)
	{
		position[deadIndex] = position[last];
		velocity[deadIndex] = velocity[last];
		acceleration[deadIndex] = acceleration[last];
		damping[deadIndex] = damping[last];
		currentLife[deadIndex] = currentLife[last];
		maxLife[deadIndex] = maxLife[last];
		startScale[deadIndex] = startScale[last];
		endScale[deadIndex] = endScale[last];
		rotation[deadIndex] = rotation[last];
		rotationSpeed[deadIndex] = rotationSpeed[last];
		startColor[deadIndex] = startColor[last];
		endColor[deadIndex] = endColor[last];
	}
	--activeCount;
}

void ParticleSystem::Reset()
{
	m_Pools.clear();
	m_PackedInstanceData.clear();
	m_Batches.clear();
}

void ParticleSystem::ResetEmitter(entt::entity entity)
{
	auto it = m_Pools.find(entity);
	if (it != m_Pools.end())
		it->second.activeCount = 0;

	if (auto* emitter = m_Scene.GetRegistry().try_get<ParticleEmitterComponent>(entity))
	{
		emitter->isFinished = false;
		emitter->timeElapsed = 0.0f;
		emitter->emissionTimer = 0.0f;
		emitter->cycleSpawnCount = 0;
		emitter->emitting = true;
	}
}

void ParticleSystem::Update(float dt)
{
	entt::registry& registry = m_Scene.GetRegistry();

	// remove unused pools
	for (auto it = m_Pools.begin(); it != m_Pools.end();)
	{
		if (!registry.valid(it->first) || !registry.any_of<ParticleEmitterComponent>(it->first))
			it = m_Pools.erase(it);
		else
			++it;
	}

	m_PackedInstanceData.clear();
	m_Batches.clear();

	auto view = registry.view<ParticleEmitterComponent, TransformComponent>();
	for (auto entity : view)
	{
		auto& emitter = view.get<ParticleEmitterComponent>(entity);
		const auto& transform = view.get<TransformComponent>(entity);

		UpdateEmitter(entity, emitter, transform, dt);
	}
}

void ParticleSystem::UpdateEmitter(entt::entity entity, ParticleEmitterComponent& emitter, const TransformComponent& transform, float dt)
{
	EmitterParticlePool& pool = m_Pools[entity];
	if (pool.capacity != emitter.amount)
		pool.Resize(emitter.amount);

	float effectiveDt = dt * emitter.speedScale;

	UpdateParticles(emitter,pool,effectiveDt);
	
	UpdateSpawning(emitter, transform, pool, effectiveDt);

	BuildParticleBatch(entity,emitter,pool,transform);
	
}

void ParticleSystem::SpawnParticles(ParticleEmitterComponent& emitter, EmitterParticlePool& pool, const TransformComponent& transform, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i)
	{
		if (pool.activeCount >= pool.capacity) break;

		uint32_t idx = pool.activeCount++;

		pool.position[idx] = CalculateSpawnPosition(emitter, transform);
		pool.velocity[idx] = CalculateInitialVelocity(emitter, transform);

		float accelVal = RandomFloat(emitter.linearAccelMin, emitter.linearAccelMax);
		pool.acceleration[idx] = (glm::length2(pool.velocity[idx]) > 0.0001f) ? glm::normalize(pool.velocity[idx]) * accelVal : glm::vec3(0.0f);

		pool.damping[idx] = RandomFloat(emitter.dampingMin, emitter.dampingMax);
		pool.currentLife[idx] = 0.0f;

		float randLifetimeFactor = 1.0f - RandomFloat(0.0f, emitter.lifetimeRandomness);
		pool.maxLife[idx] = std::max(0.01f, emitter.lifetime * randLifetimeFactor);

		pool.startScale[idx] = RandomFloat(emitter.scaleMin, emitter.scaleMax);
		pool.endScale[idx] = emitter.scaleEnd;

		pool.rotation[idx] = glm::radians(RandomFloat(emitter.rotationMin, emitter.rotationMax));
		pool.rotationSpeed[idx] = glm::radians(RandomFloat(emitter.angularVelocityMin, emitter.angularVelocityMax));

		pool.startColor[idx] = emitter.startColor;
		pool.endColor[idx] = emitter.endColor;
	}
}

glm::vec3 ParticleSystem::CalculateSpawnPosition(const ParticleEmitterComponent& emitter, const TransformComponent& transform)
{
	glm::vec3 spawnPos(0.0f);

	switch (emitter.emitterShape)
	{
	case ParticleEmitterShape::Point:
		spawnPos = glm::vec3(0.0f);
		break;
	case ParticleEmitterShape::Sphere:
		spawnPos = glm::ballRand(emitter.shapeDimensions.x);
		break;
	case ParticleEmitterShape::SphereSurface:
		spawnPos = glm::sphericalRand(emitter.shapeDimensions.x);
		break;
	case ParticleEmitterShape::Box:
		spawnPos = glm::vec3(
			RandomFloat(-emitter.shapeDimensions.x, emitter.shapeDimensions.x),
			RandomFloat(-emitter.shapeDimensions.y, emitter.shapeDimensions.y),
			RandomFloat(-emitter.shapeDimensions.z, emitter.shapeDimensions.z)
		);
		break;
	}

	if (!emitter.localCoords)
		spawnPos += transform.GetPosition();

	return spawnPos;
}

glm::vec3 ParticleSystem::CalculateInitialVelocity(const ParticleEmitterComponent& emitter, const TransformComponent& transform)
{
	glm::vec3 baseDir = glm::normalize(emitter.direction);
	if (glm::length2(baseDir) < 0.001f)
		baseDir = glm::vec3(0.0f, 1.0f, 0.0f);

	if (!emitter.localCoords)
	{
		glm::quat rot = glm::quat(glm::radians(transform.GetEulerRotation()));
		baseDir = rot * baseDir;
	}

	float spreadRad = glm::radians(emitter.spread * 0.5f);
	glm::vec3 randDir = glm::ballRand(1.0f);
	glm::vec3 finalDir = glm::normalize(glm::mix(baseDir, randDir, glm::sin(spreadRad)));

	float speed = RandomFloat(emitter.linearVelocityMin, emitter.linearVelocityMax);
	return finalDir * speed;
}

void ParticleSystem::BuildParticleBatch(entt::entity entity, ParticleEmitterComponent& emitter, EmitterParticlePool& pool, const TransformComponent& transform)
{
	if (pool.activeCount > 0)
	{
		ParticleBatch batch;
		batch.entity = entity;
		batch.instanceOffset = static_cast<uint32_t>(m_PackedInstanceData.size());
		batch.instanceCount = pool.activeCount;
		batch.worldPosition = transform.GetPosition();
		batch.localCoords = emitter.localCoords;
		m_Batches.push_back(batch);

		m_PackedInstanceData.reserve(m_PackedInstanceData.size() + pool.activeCount);

		glm::mat4 worldMatrix = emitter.localCoords ? transform.GetWorldMatrix() : glm::mat4(1.0f);

		for (uint32_t i = 0; i < pool.activeCount; ++i)
		{
			float t = glm::clamp(pool.currentLife[i] / pool.maxLife[i], 0.0f, 1.0f);

			float currentScale = glm::mix(pool.startScale[i], pool.endScale[i], t);
			glm::vec4 currentColor = glm::mix(pool.startColor[i], pool.endColor[i], t);

			glm::vec3 finalPos = pool.position[i];
			if (emitter.localCoords)
				finalPos = glm::vec3(worldMatrix * glm::vec4(finalPos, 1.0f));

			ParticleInstanceData instance;
			instance.positionAndScale = glm::vec4(finalPos, currentScale);
			instance.color = currentColor;
			instance.rotationAndCustom = glm::vec4(pool.rotation[i], 0.0f, 0.0f, 0.0f);

			m_PackedInstanceData.push_back(instance);
		}
	}
}

void ParticleSystem::UpdateSpawning(ParticleEmitterComponent& emitter, const TransformComponent& transform ,EmitterParticlePool& pool, float effectiveDt)
{
	if (emitter.emitting && !emitter.isFinished)
	{
		emitter.timeElapsed += effectiveDt;

		float cycleLength = std::max(emitter.lifetime, 0.0001f);

		emitter.emissionTimer += effectiveDt;

		bool cycleCompleted = false;
		if (!emitter.oneShot)
		{
			while (emitter.emissionTimer >= cycleLength)
			{
				emitter.emissionTimer -= cycleLength;
				emitter.cycleSpawnCount = 0;
			}
		}
		else if (emitter.emissionTimer >= cycleLength)
		{
			emitter.emissionTimer = cycleLength;
			cycleCompleted = true;
		}

		float phase = glm::clamp(emitter.emissionTimer / cycleLength, 0.0f, 1.0f);

		float activeWindow = glm::clamp(1.0f - emitter.explosiveness, 0.0001f, 1.0f);
		float progress = glm::clamp(phase / activeWindow, 0.0f, 1.0f);

		uint32_t targetSpawned = static_cast<uint32_t>(progress * static_cast<float>(emitter.amount));

		if (emitter.randomness > 0.0f && targetSpawned > emitter.cycleSpawnCount)
		{
			float jitter = RandomFloat(-emitter.randomness, emitter.randomness) * 0.5f;
			int32_t jittered = static_cast<int32_t>(targetSpawned) + static_cast<int32_t>(jitter * emitter.amount);
			targetSpawned = static_cast<uint32_t>(glm::clamp(jittered, 0, static_cast<int32_t>(emitter.amount)));
		}

		if (targetSpawned > emitter.cycleSpawnCount)
		{
			uint32_t spawnCount = targetSpawned - emitter.cycleSpawnCount;
			emitter.cycleSpawnCount = targetSpawned;

			uint32_t availableSlots = pool.capacity - pool.activeCount;
			uint32_t toSpawn = std::min(spawnCount, availableSlots);
			if (toSpawn > 0)
				SpawnParticles(emitter, pool, transform, toSpawn);
		}

		if (emitter.oneShot && cycleCompleted)
			emitter.isFinished = true;
	}

	if (emitter.oneShot && emitter.isFinished && pool.activeCount == 0)
	{
		emitter.emitting = false;
	}

}

void ParticleSystem::UpdateParticles(ParticleEmitterComponent& emitter, EmitterParticlePool& pool, float effectiveDt)
{
	for (uint32_t i = 0; i < pool.activeCount; )
	{
		pool.currentLife[i] += effectiveDt;

		if (pool.currentLife[i] >= pool.maxLife[i])
		{
			pool.SwapAndPop(i);
			continue;
		}

		pool.velocity[i] += (pool.acceleration[i] + emitter.gravity) * effectiveDt;
		pool.velocity[i] *= glm::clamp(1.0f - pool.damping[i] * effectiveDt, 0.0f, 1.0f);
		pool.position[i] += pool.velocity[i] * effectiveDt;
		pool.rotation[i] += pool.rotationSpeed[i] * effectiveDt;

		++i;
	}
}
