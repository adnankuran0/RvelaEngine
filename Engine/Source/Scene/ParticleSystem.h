#pragma once
#include "Core/Ref.h"
#include "Scene/Components/ParticleEmitterComponent.h"
#include "Scene/Components.h"
#include "entt/entt.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

namespace rv {

class Scene;

struct ParticleInstanceData
{
	glm::vec4 positionAndScale;
	glm::vec4 color;
	glm::vec4 rotationAndCustom;
};

struct EmitterParticlePool
{
	uint32_t capacity = 0;
	uint32_t activeCount = 0;

	std::vector<glm::vec3> position;
	std::vector<glm::vec3> velocity;
	std::vector<glm::vec3> acceleration;
	std::vector<float> damping;

	std::vector<float> currentLife;
	std::vector<float> maxLife;

	std::vector<float> startScale;
	std::vector<float> endScale;

	std::vector<float> rotation;
	std::vector<float> rotationSpeed;

	std::vector<glm::vec4> startColor;
	std::vector<glm::vec4> endColor;

	void Resize(uint32_t newCapacity);
	void SwapAndPop(uint32_t deadIndex);
};

struct ParticleBatch
{
	entt::entity entity = entt::null;
	uint32_t instanceOffset = 0;
	uint32_t instanceCount = 0;
	glm::vec3 worldPosition = glm::vec3(0.0f);
	bool localCoords = false;
};

class ParticleSystem
{
public:
	ParticleSystem(Scene& scene) : m_Scene(scene) {}
	~ParticleSystem() = default;

	void Update(float dt);

	const std::vector<ParticleInstanceData>& GetPackedInstanceBuffer() const { return m_PackedInstanceData; }
	const std::vector<ParticleBatch>& GetBatches() const { return m_Batches; }

	void Reset();
	void ResetEmitter(entt::entity entity);

private:
	void UpdateEmitter(entt::entity entity, ParticleEmitterComponent& emitter, const TransformComponent& transform, float dt);
	void SpawnParticles(ParticleEmitterComponent& emitter, EmitterParticlePool& pool, const TransformComponent& transform, uint32_t count);
	glm::vec3 CalculateSpawnPosition(const ParticleEmitterComponent& emitter, const TransformComponent& transform);
	glm::vec3 CalculateInitialVelocity(const ParticleEmitterComponent& emitter, const TransformComponent& transform);
	void BuildParticleBatch(entt::entity entity, ParticleEmitterComponent& emitter, EmitterParticlePool& pool,const TransformComponent& transform);
	void UpdateSpawning(ParticleEmitterComponent& emitter, const TransformComponent& transform, EmitterParticlePool& pool, float effectiveDt);
	void UpdateParticles(ParticleEmitterComponent& emitter, EmitterParticlePool& pool, float effectiveDt);

private:
	Scene& m_Scene;
	std::unordered_map<entt::entity, EmitterParticlePool> m_Pools;

	std::vector<ParticleInstanceData> m_PackedInstanceData;
	std::vector<ParticleBatch> m_Batches;
};

}