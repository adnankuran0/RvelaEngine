#pragma once
#include "Jolt/Jolt.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include "ObjectLayerPairFilter.h"
#include "BroadPhaseLayer.h"
#include "ObjectVsBroadPhaseLayerFilter.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "ContactListener.h"
#include "CharacterContactListener.h"
#include "BodyActivationListener.h"
#include "entt/entt.h"
#include "ShapeBuilder.h"
#include "PhysicsWorld.h"
#include "Renderer/JoltDebugRenderer.h"

namespace rv {

class Scene;
struct RigidbodyComponent;
struct CharacterBodyComponent;
struct PhysicsConstraintComponent;

struct PhysicsStats
{
	uint32_t totalBodies = 0;
	uint32_t activeBodies = 0;
	uint32_t staticBodies = 0;
	uint32_t dynamicBodies = 0;
	uint32_t kinematicBodies = 0;
	uint32_t characterBodies = 0;
	uint32_t totalColliders = 0;
	float stepTimeMs = 0.0f;
};

class PhysicsSystem
{
public:
	PhysicsSystem(Scene& scene);
	void OnStart();
	void OnStop();
	void Step(float dt);
	void Update();
	PhysicsStats GetStats();
	inline Physics::PhysicsWorld& GetPhysicsWorld() { return m_PhysicsWorld; }
	std::vector<Physics::CollisionEvent> FlushEvents()
	{
		std::lock_guard lock(m_CollisionEventMutex);
		std::vector<Physics::CollisionEvent> events;
		events.swap(m_CollisionEventQueue);
		return events;
	}

private:
	void BindCallbacks();

	void BuildBodies();
	void RebuildDirtyShapes();
	void BuildRigidbodies();
	void BuildCharacterBodies();
	void BuildConstraints();

	void SyncBodiesFromTransforms();
	void SyncTransforms();
	void SyncBodyTransforms();
	void SyncCharacterTransforms();

	JPH::BodyCreationSettings BuildBodyCreationSettings(RigidbodyComponent& rbComp, TransformComponent& tComp);
	JPH::Ref<JPH::CharacterVirtualSettings> BuildCharacterBodyCreationSettings(CharacterBodyComponent& cbComp, TransformComponent& tComp);
	void UpdateCharacters(float dt); // TODO: think about seperate MoveAndSlide function
	inline JPH::BodyInterface& BodyInterface() { return m_PhysicsSystem.GetBodyInterface(); }

	bool HasAnyCollider(entt::registry& reg, entt::entity e);
	entt::entity FindAttachedBodyEntity(entt::registry& reg, entt::entity e);
	bool IsAnyColliderDirty(entt::entity e);
	bool IsAnyColliderInHierarchyDirty(entt::entity root);
	void ClearColliderDirtyFlags(entt::entity e);
	void ClearColliderDirtyFlagsInHierarchy(entt::entity root);
	void CheckAndUpdateScaleDirty(entt::entity e, glm::vec3& scaleCache,bool isCharacter);
	bool CheckAndUpdateHierarchyTransformsDirty(entt::entity root, bool isCharacter);

	void OnRigidbodyDestroyed(entt::registry& reg, entt::entity e);
	void OnCharacterBodyDestroyed(entt::registry& reg, entt::entity e);
	void OnConstraintDestroyed(entt::registry& reg, entt::entity e);
	void OnShapeChanged(entt::registry& reg, entt::entity e);

	void DebugDraw();
private:
	const unsigned int cMaxBodies = 1024;
	const unsigned int cNumBodyMutexes = 0;
	const unsigned int cMaxBodyPairs = 1024;
	const unsigned int cMaxContactConstraints = 1024;
	const int cCollisionSteps = 1;

	Scene& m_Scene;

	JPH::TempAllocatorImpl m_TempAllocator;
	JPH::JobSystemThreadPool m_JobSystem;

	Physics::BPLayerInterfaceImpl m_BroadPhaseLayerInterface;
	Physics::ObjectVsBroadPhaseLayerFilterImpl m_ObjectVsBroadPhaseLayerFilter;
	Physics::ObjectLayerPairFilterImpl m_ObjectVsObjectLayerFilter;
	JPH::CharacterVsCharacterCollisionSimple m_CharacterVsCharacterCollision;

	JPH::PhysicsSystem m_PhysicsSystem;

	Physics::BodyActivationListener m_BodyActivationListener;
	Physics::ContactListener m_ContactListener;
	Physics::CharacterContactListener m_CharacterContactListener;

	Physics::ShapeBuilder m_ShapeBuilder;

	Physics::PhysicsWorld m_PhysicsWorld;

	std::mutex m_CollisionEventMutex;
	std::vector<Physics::CollisionEvent> m_CollisionEventQueue;

	JoltDebugRenderer m_DebugRenderer;

	struct ColliderTransformSnapshot
	{
		entt::entity entity = entt::null;
		glm::vec3 relPos{ 0.0f };
		glm::quat relRot{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 relScale{ 1.0f };
	};

	std::unordered_map<entt::entity, std::vector<ColliderTransformSnapshot>> m_ColliderHierarchyCache;
	float m_StepTimeMs = 0.0f;
};

}
