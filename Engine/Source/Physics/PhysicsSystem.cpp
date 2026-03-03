#include "rvelapch.h"
#include "PhysicsSystem.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include "Layers.h"
#include <iostream>
#include <cstdarg>
#include <thread>
#include "Scene/Scene.h"
#include <Scene/Components/RigidbodyComponent.h>
#include "Math/RvelaMath.h"

JPH_SUPPRESS_WARNINGS

using namespace rv;

PhysicsSystem::PhysicsSystem(Scene& scene) :
	m_Scene(scene),
	m_TempAllocator(10 * 1024 * 1024),
	m_JobSystem(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1)
{
	m_PhysicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_BroadPhaseLayerInterface, m_ObjectVsBroadPhaseLayerFilter, m_ObjectVsObjectLayerFilter);

	m_PhysicsSystem.SetBodyActivationListener(&m_BodyActivationListener);
	m_PhysicsSystem.SetContactListener(&m_ContactListener);


	JPH::BodyCreationSettings settings;
	
}

void PhysicsSystem::Step(float dt)
{
	m_PhysicsSystem.Update(dt, cCollisionSteps, &m_TempAllocator, &m_JobSystem);
	SyncTransforms();
}

void PhysicsSystem::OnStart()
{
	m_PhysicsSystem.OptimizeBroadPhase();
	InitBodies();
}

void PhysicsSystem::InitBodies()
{
	auto view = m_Scene.GetRegistry().view<RigidbodyComponent>();
	for (auto& e : view)
	{
		auto& rb = m_Scene.GetComponent<RigidbodyComponent>(e);
		auto& transform = m_Scene.GetComponent<TransformComponent>(e);
		
		JPH::BoxShapeSettings shapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f));
		JPH::ShapeRefC shape = shapeSettings.Create().Get();

		JPH::EMotionType motionType = JPH::EMotionType::Dynamic;
		JPH::ObjectLayer layer = Physics::Layers::MOVING;

		if (rb.bodyType == Physics::BodyType::STATIC) {
			motionType = JPH::EMotionType::Static;
			layer = Physics::Layers::NON_MOVING;
		}
		else if (rb.bodyType == Physics::BodyType::KINEMATIC) {
			motionType = JPH::EMotionType::Kinematic;
		}

		JPH::BodyCreationSettings settings;
		settings.SetShape(shape);
		settings.mPosition = math::ToJoltRVec3(transform.GetWorldPosition());
		settings.mRotation = math::ToJoltQuat(transform.GetWorldRotation());
		settings.mMotionType = motionType;
		settings.mObjectLayer = layer;

		settings.mFriction = rb.friction;
		settings.mRestitution = rb.restitution;
		settings.mLinearDamping = rb.linearDamping;
		settings.mAngularDamping = rb.angularDamping;
		settings.mGravityFactor = rb.gravityFactor;
		settings.mIsSensor = rb.isSensor;

		JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::All;

		if (!rb.lockRotationX) dofs = dofs | JPH::EAllowedDOFs::RotationX;
		if (!rb.lockRotationY) dofs = dofs | JPH::EAllowedDOFs::RotationY;
		if (!rb.lockRotationZ) dofs = dofs | JPH::EAllowedDOFs::RotationZ;
		if (!rb.lockTranslationX) dofs = dofs | JPH::EAllowedDOFs::TranslationX;
		if (!rb.lockTranslationY) dofs = dofs | JPH::EAllowedDOFs::TranslationY;
		if (!rb.lockRotationZ) dofs = dofs | JPH::EAllowedDOFs::TranslationZ;
		settings.mAllowedDOFs = dofs;

		if (rb.useCCD) {
			settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
		}

		JPH::BodyInterface& bodyInterface = m_PhysicsSystem.GetBodyInterface();
		rb.RuntimeBodyID = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::Activate);

		
	}
}

void rv::PhysicsSystem::SyncTransforms()
{
	JPH::BodyInterface& bodyInterface = m_PhysicsSystem.GetBodyInterface();
	auto view = m_Scene.GetRegistry().view<RigidbodyComponent, TransformComponent, SceneTreeComponent>();

	for (auto& e : view)
	{
		auto& rb = view.get<RigidbodyComponent>(e);
		if (rb.bodyType == Physics::BodyType::STATIC) continue;

		glm::vec3 worldPos = math::FromJoltRVec3(bodyInterface.GetPosition(rb.RuntimeBodyID));
		glm::quat worldRot = math::FromJoltQuat(bodyInterface.GetRotation(rb.RuntimeBodyID));

		TransformComponent& transform = view.get<TransformComponent>(e);
		SceneTreeComponent& node = view.get<SceneTreeComponent>(e);

		if (node.parent != entt::null && node.parent != m_Scene.GetRootEntity())
		{
			auto& parentTransform = m_Scene.GetComponent<TransformComponent>(node.parent);
			glm::mat4 invParentWorld = glm::inverse(parentTransform.GetWorldMatrix());

			glm::vec4 localPos = invParentWorld * glm::vec4(worldPos, 1.0f);
			glm::quat localRot = glm::inverse(parentTransform.GetWorldRotation()) * worldRot;

			transform.SetPosition(glm::vec3(localPos));
			transform.SetRotation(localRot);
		}
		else
		{
			transform.SetPosition(worldPos);
			transform.SetRotation(worldRot);
		}

		transform.SetWorldTransform(worldPos, worldRot, transform.GetWorldScale());
	}
}
