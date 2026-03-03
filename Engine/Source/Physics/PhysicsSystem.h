#pragma once
#include "Jolt/Jolt.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include "ObjectLayerPairFilter.h"
#include "BroadPhaseLayer.h"
#include "ObjectVsBroadPhaseLayerFilter.h"
#include <memory>
#include "ContactListener.h"
#include "BodyActivationListener.h"

namespace rv {

class Scene;

class PhysicsSystem
{
public:
	PhysicsSystem(Scene& scene);
	void OnStart();
	void Step(float dt);

private:
	void InitBodies();

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

	JPH::PhysicsSystem m_PhysicsSystem;

	Physics::BodyActivationListener m_BodyActivationListener;
	Physics::ContactListener m_ContactListener;


	Scene& m_Scene;
};

}