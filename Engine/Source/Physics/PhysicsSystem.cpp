#include "rvelapch.h"
#include "PhysicsSystem.h"
#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>


#include "Layers.h"
#include <iostream>
#include <cstdarg>
#include <thread>
#include "Scene/Scene.h"
#include <Scene/Components/RigidbodyComponent.h>

#include "Math/RvelaMath.h"
#include <glm/gtx/component_wise.hpp>
#include "ShapeBuilder.h"

JPH_SUPPRESS_WARNINGS

using namespace rv;

PhysicsSystem::PhysicsSystem(Scene& scene) :
	m_Scene(scene),
	m_TempAllocator(10 * 1024 * 1024),
	m_JobSystem(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1)
{
	m_PhysicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, 
		m_BroadPhaseLayerInterface, m_ObjectVsBroadPhaseLayerFilter, m_ObjectVsObjectLayerFilter);

	m_PhysicsSystem.SetBodyActivationListener(&m_BodyActivationListener);
	m_PhysicsSystem.SetContactListener(&m_ContactListener);

	m_PhysicsWorld.Init(m_PhysicsSystem);
}

void PhysicsSystem::Step(float dt)
{
	InitialiseBodies();
	UpdateCharacters(dt);
	m_PhysicsSystem.Update(dt, cCollisionSteps, &m_TempAllocator, &m_JobSystem);
	SyncTransforms();
}


void PhysicsSystem::OnStart()
{
	m_PhysicsSystem.OptimizeBroadPhase();
	InitialiseBodies();
}

void PhysicsSystem::InitialiseBodies()
{
	auto rbView = m_Scene.GetRegistry().view<RigidbodyComponent>();
	for (auto& e : rbView)
	{
		auto& rb = m_Scene.GetComponent<RigidbodyComponent>(e);
		if (!rb.RuntimeBodyID.IsInvalid()) continue;

		auto& transform = m_Scene.GetComponent<TransformComponent>(e);
		JPH::BodyCreationSettings settings = BuildBodyCreationSettings(rb, transform);

		JPH::ShapeRefC shape = m_ShapeBuilder.BuildShape(m_Scene.GetRegistry(),e);
		settings.SetShape(shape);

		rb.RuntimeBodyID = BodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);
	}

	auto cbView = m_Scene.GetRegistry().view<CharacterBodyComponent>();
	for (auto& e : cbView)
	{
		auto& cb = m_Scene.GetComponent<CharacterBodyComponent>(e);
		if (cb.character) continue; 

		auto& transform = m_Scene.GetComponent<TransformComponent>(e);

		auto settings = BuildCharacterBodyCreationSettings(cb, transform);
		settings->mShape = m_ShapeBuilder.BuildShape(m_Scene.GetRegistry(), e);

		cb.character = new JPH::CharacterVirtual(
			settings,
			math::ToJoltVec3(transform.GetWorldPosition()),
			math::ToJoltQuat(transform.GetWorldRotation()),
			(int)e,
			&m_PhysicsSystem 
		);
		cb.character->SetCharacterVsCharacterCollision(&m_CharacterVsCharacterCollision);
		m_CharacterVsCharacterCollision.Add(cb.character);
		cb.character->SetListener(&m_CharacterContactListener);
	}

}

void PhysicsSystem::SyncTransforms()
{
	SyncBodyTransforms();
	SyncCharacterTransforms();
}

void rv::PhysicsSystem::SyncBodyTransforms()
{
	auto view = m_Scene.GetRegistry().view<RigidbodyComponent, TransformComponent, SceneTreeComponent>();
	for (auto e : view)
	{
		auto& rb = view.get<RigidbodyComponent>(e);
		if (rb.bodyType == Physics::BodyType::STATIC) continue;

		auto& transform = view.get<TransformComponent>(e);
		auto& node = view.get<SceneTreeComponent>(e);

		glm::vec3 worldPos = math::FromJoltRVec3(BodyInterface().GetPosition(rb.RuntimeBodyID));
		glm::quat worldRot = math::FromJoltQuat(BodyInterface().GetRotation(rb.RuntimeBodyID));

		if (node.parent == entt::null || node.parent == m_Scene.GetRootEntity())
		{
			transform.SetPosition(worldPos);
			transform.SetRotation(worldRot);
		}
		else
		{
			auto& parentTransform = m_Scene.GetComponent<TransformComponent>(node.parent);
			glm::mat4 invParentWorld = glm::inverse(parentTransform.GetWorldMatrix());

			glm::vec3 localPos = glm::vec3(invParentWorld * glm::vec4(worldPos, 1.0f));
			glm::quat localRot = glm::inverse(parentTransform.GetWorldRotation()) * worldRot;

			transform.SetPosition(localPos);
			transform.SetRotation(localRot);
		}

		glm::mat4 newWorld = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot) * glm::scale(glm::mat4(1.0f), transform.GetScale());
		transform.SetWorldMatrix(newWorld);
	}
}

void rv::PhysicsSystem::SyncCharacterTransforms()
{
	auto view = m_Scene.GetRegistry().view<CharacterBodyComponent, TransformComponent>();
	for (auto& e : view)
	{
		auto& cb = view.get<CharacterBodyComponent>(e);
		if (!cb.character) continue;

		auto& transform = view.get<TransformComponent>(e);

		glm::vec3 pos = math::FromJoltRVec3(cb.character->GetPosition());
		glm::quat rot = math::FromJoltQuat(cb.character->GetRotation());

		transform.SetPosition(pos);
		transform.SetRotation(rot);

		glm::mat4 newWorld = glm::translate(glm::mat4(1.0f), pos)
			* glm::mat4_cast(rot)
			* glm::scale(glm::mat4(1.0f), transform.GetScale());
		transform.SetWorldMatrix(newWorld);
	}
}

JPH::BodyCreationSettings PhysicsSystem::BuildBodyCreationSettings(RigidbodyComponent& rbComp, TransformComponent& tComp)
{
	JPH::BodyCreationSettings settings;
	JPH::EMotionType motionType = JPH::EMotionType::Dynamic;
	JPH::ObjectLayer layer = Physics::Layers::MOVING;

	if (rbComp.bodyType == Physics::BodyType::STATIC) {
		motionType = JPH::EMotionType::Static;
		layer = Physics::Layers::NON_MOVING;
	}
	else if (rbComp.bodyType == Physics::BodyType::KINEMATIC) {
		motionType = JPH::EMotionType::Kinematic;
	}

	settings.mPosition = math::ToJoltRVec3(tComp.GetWorldPosition());
	settings.mRotation = math::ToJoltQuat(tComp.GetWorldRotation());
	settings.mMotionType = motionType;
	settings.mObjectLayer = layer;

	settings.mFriction = rbComp.friction;
	settings.mRestitution = rbComp.restitution;
	settings.mLinearDamping = rbComp.linearDamping;
	settings.mAngularDamping = rbComp.angularDamping;
	settings.mGravityFactor = rbComp.gravityFactor;
	settings.mIsSensor = rbComp.isSensor;

	if (!rbComp.autoCalculateMass)
	{
		settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		settings.mMassPropertiesOverride.mMass = rbComp.mass;
	}
	//TODO: override mass and innertia for bodies with mesh colliders


	settings.mMaxLinearVelocity = rbComp.maxLinearVelocity;
	settings.mMaxAngularVelocity = rbComp.maxAngularVelocity;
	
	settings.mAllowSleeping = rbComp.allowSleep;

	// ensure sensors detect static bodies
	settings.mCollideKinematicVsNonDynamic = true;
	

	JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::None;

	if (!rbComp.lockRotationX) dofs = dofs | JPH::EAllowedDOFs::RotationX;
	if (!rbComp.lockRotationY) dofs = dofs | JPH::EAllowedDOFs::RotationY;
	if (!rbComp.lockRotationZ) dofs = dofs | JPH::EAllowedDOFs::RotationZ;
	if (!rbComp.lockTranslationX) dofs = dofs | JPH::EAllowedDOFs::TranslationX;
	if (!rbComp.lockTranslationY) dofs = dofs | JPH::EAllowedDOFs::TranslationY;
	if (!rbComp.lockTranslationZ) dofs = dofs | JPH::EAllowedDOFs::TranslationZ;
	settings.mAllowedDOFs = dofs;

	if (rbComp.useCCD) {
		settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
	}
	return settings;
}

JPH::Ref<JPH::CharacterVirtualSettings> rv::PhysicsSystem::BuildCharacterBodyCreationSettings(CharacterBodyComponent& cbComp, TransformComponent& tComp)
{
	JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
	settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.5f);
	settings->mMass = cbComp.mass;
	settings->mMaxStrength = cbComp.maxStrength;
	settings->mShapeOffset = math::ToJoltVec3(cbComp.shapeOffset);
	settings->mPredictiveContactDistance = cbComp.predictiveContactDistance;
	settings->mMaxSlopeAngle = JPH::DegreesToRadians(cbComp.maxSlopeAngle);
	return settings;
}

void rv::PhysicsSystem::UpdateCharacters(float dt)
{
	auto cbView = m_Scene.GetRegistry().view<CharacterBodyComponent>();
	for (auto& e : cbView)
	{
		auto& cb = m_Scene.GetComponent<CharacterBodyComponent>(e);
		if (!cb.character) continue;

		

		cb.character->UpdateGroundVelocity();

		JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
		//updateSettings.mStickToFloorStepDown = -cb.character->GetUp() * 0.5f; 
		//updateSettings.mWalkStairsStepUp = cb.character->GetUp() * 0.4f;

		cb.character->ExtendedUpdate(
			dt,
			-cb.character->GetUp() * m_PhysicsSystem.GetGravity().Length(),
			updateSettings,
			m_PhysicsSystem.GetDefaultBroadPhaseLayerFilter(Physics::Layers::MOVING),
			m_PhysicsSystem.GetDefaultLayerFilter(Physics::Layers::MOVING),
			{},
			{},
			m_TempAllocator
		);

	}
}



