#pragma once
#include "Jolt/Jolt.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include "ObjectLayerPairFilter.h"
#include "BroadPhaseLayer.h"
#include "ObjectVsBroadPhaseLayerFilter.h"
#include <memory>
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

class PhysicsSystem
{
public:
	PhysicsSystem(Scene& scene);
	void OnStart();
	void Step(float dt);
	void Update();
	inline Physics::PhysicsWorld& GetPhysicsWorld() { return m_PhysicsWorld; }
	std::vector<Physics::CollisionEvent> FlushEvents() { return std::move(m_CollisionEventQueue); }

private:
	void BindCallbacks();

	void BuildBodies();
	void RebuildDirtyShapes();
	void BuildRigidbodies();
	void BuildCharacterBodies();

	void SyncBodiesFromTransforms();
	void SyncTransforms();
	void SyncBodyTransforms();
	void SyncCharacterTransforms();

	JPH::BodyCreationSettings BuildBodyCreationSettings(RigidbodyComponent& rbComp, TransformComponent& tComp);
	JPH::Ref<JPH::CharacterVirtualSettings> BuildCharacterBodyCreationSettings(CharacterBodyComponent& cbComp, TransformComponent& tComp);
	void UpdateCharacters(float dt); // TODO: think about seperate MoveAndSlide function
	inline JPH::BodyInterface& BodyInterface() { return m_PhysicsSystem.GetBodyInterface(); }

	bool IsAnyColliderDirty(entt::entity e);
	void ClearColliderDirtyFlags(entt::entity e);
	void CheckAndUpdateScaleDirty(entt::entity e, glm::vec3& scaleCache,bool isCharacter);

	void OnRigidbodyDestroyed(entt::registry& reg, entt::entity e);
	void OnCharacterBodyDestroyed(entt::registry& reg, entt::entity e);
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

	std::vector<Physics::CollisionEvent> m_CollisionEventQueue;

	JoltDebugRenderer m_DebugRenderer;
	

};

}