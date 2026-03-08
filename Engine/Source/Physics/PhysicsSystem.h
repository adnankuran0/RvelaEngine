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
	inline Physics::PhysicsWorld& GetPhysicsWorld() { return m_PhysicsWorld; }

private:
	void BuildBodies();
	void BuildRigidbodies();
	void BuildCharacterBodies();

	void SyncTransforms();
	void SyncBodyTransforms();
	void SyncCharacterTransforms();
	JPH::BodyCreationSettings BuildBodyCreationSettings(RigidbodyComponent& rbComp, TransformComponent& tComp);
	JPH::Ref<JPH::CharacterVirtualSettings> BuildCharacterBodyCreationSettings(CharacterBodyComponent& cbComp, TransformComponent& tComp);
	void UpdateCharacters(float dt); // TODO: think about seperate MoveAndSlide function
	inline JPH::BodyInterface& BodyInterface() { return m_PhysicsSystem.GetBodyInterface(); }

private:
	const unsigned int cMaxBodies = 1024;
	const unsigned int cNumBodyMutexes = 0;
	const unsigned int cMaxBodyPairs = 1024;
	const unsigned int cMaxContactConstraints = 1024;
	const int cCollisionSteps = 1;

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

	Scene& m_Scene;
};

}