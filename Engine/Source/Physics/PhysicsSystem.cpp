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
#include "UserData.h"

#include "Renderer/DebugRenderer.h"

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

	m_CollisionEventQueue.reserve(50);
	m_ContactListener.Init(&m_PhysicsSystem,&m_CollisionEventQueue);
	m_CharacterContactListener.Init(&m_PhysicsSystem, &m_CollisionEventQueue);

	BindCallbacks();
}

void PhysicsSystem::Step(float dt)
{
	BuildBodies();

	if (m_Scene.GetState() == SceneState::PLAY)
		SyncBodiesFromTransforms();

	UpdateCharacters(dt);

	m_PhysicsSystem.Update(dt, cCollisionSteps, &m_TempAllocator, &m_JobSystem);

	SyncTransforms();
}

void PhysicsSystem::DebugDraw()
{
	JPH::BodyManager::DrawSettings settings;
	settings.mDrawShapeWireframe = true;

	// draw rigidbodies
	m_PhysicsSystem.DrawBodies(settings, &m_DebugRenderer);

	// draw character bodies
	auto view = m_Scene.GetRegistry().view<CharacterBodyComponent>();
	for (auto e : view)
	{
		auto& cb = view.get<CharacterBodyComponent>(e);
		if (!cb.character) continue;

		JPH::RVec3 pos = cb.character->GetPosition();
		JPH::Quat rot = cb.character->GetRotation();
		JPH::ShapeRefC shape = cb.character->GetShape();

		JPH::Mat44 transform = JPH::Mat44::sRotationTranslation(rot, pos);

		shape->Draw(
			&m_DebugRenderer,
			transform,
			JPH::Vec3::sReplicate(1.0f),
			JPH::Color::sGreen,
			false,
			false
		);
	}
}


void PhysicsSystem::Update()
{
	BuildBodies();
	if (DebugRenderer::Get().GetSettings().drawColliders)
		DebugDraw();

	if (m_Scene.GetState() == SceneState::EDIT)
		SyncBodiesFromTransforms();
}

void PhysicsSystem::OnStart()
{
	BuildBodies();
	m_PhysicsSystem.OptimizeBroadPhase();
}

void PhysicsSystem::BindCallbacks()
{
	auto& reg = m_Scene.GetRegistry();

	reg.on_destroy<RigidbodyComponent>().connect<&PhysicsSystem::OnRigidbodyDestroyed>(this);
	reg.on_destroy<CharacterBodyComponent>().connect<&PhysicsSystem::OnCharacterBodyDestroyed>(this);

	reg.on_destroy<BoxColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_construct<BoxColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_destroy<SphereColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_construct<SphereColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_destroy<CapsuleColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_construct<CapsuleColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_destroy<CylinderColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_construct<CylinderColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_destroy<MeshColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_construct<MeshColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_destroy<ConvexHullColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
	reg.on_construct<ConvexHullColliderComponent>().connect<&PhysicsSystem::OnShapeChanged>(this);
}


void PhysicsSystem::BuildBodies()
{
	RebuildDirtyShapes();
	BuildRigidbodies();
	BuildCharacterBodies();
}

void PhysicsSystem::RebuildDirtyShapes()
{
	auto rbView = m_Scene.GetRegistry().view<RigidbodyComponent>();
	for (auto e : rbView)
	{
		auto& rb = m_Scene.GetRegistry().get<RigidbodyComponent>(e);
		if (rb.RuntimeBodyID.IsInvalid()) continue;

		auto& tc = m_Scene.GetRegistry().get<TransformComponent>(e);
		glm::vec3 worldScale = tc.GetWorldScale();

		CheckAndUpdateScaleDirty(e, rb.worldScaleCache, false);

		bool dirty = rb.IsShapeDirty() || IsAnyColliderDirty(e);
		if (!dirty) continue;

		JPH::ShapeRefC newShape = m_ShapeBuilder.BuildShape(m_Scene.GetRegistry(), e);
		BodyInterface().SetShape(rb.RuntimeBodyID, newShape, false, JPH::EActivation::Activate);

		ClearColliderDirtyFlags(e);

		rb.ClearShapeDirty();

	}

	auto cbView = m_Scene.GetRegistry().view<CharacterBodyComponent>();
	for (auto e : cbView)
	{
		auto& cb = m_Scene.GetRegistry().get<CharacterBodyComponent>(e);
		if (!cb.character) continue;

		CheckAndUpdateScaleDirty(e,cb.worldScaleCache,true);

		bool dirty = cb.IsShapeDirty() || IsAnyColliderDirty(e);
		if (!dirty) continue;

		JPH::ShapeRefC newShape = m_ShapeBuilder.BuildShape(m_Scene.GetRegistry(), e);
		cb.character->SetShape(
			newShape,
			1.5f * m_PhysicsSystem.GetPhysicsSettings().mPenetrationSlop,
			m_PhysicsSystem.GetDefaultBroadPhaseLayerFilter(Physics::Layers::MOVING),
			m_PhysicsSystem.GetDefaultLayerFilter(Physics::Layers::MOVING),
			{}, {},
			m_TempAllocator
		);

		ClearColliderDirtyFlags(e);

		cb.ClearShapeDirty();
	}

	
}

void PhysicsSystem::BuildRigidbodies()
{
	auto rbView = m_Scene.GetRegistry().view<RigidbodyComponent>();
	for (auto& e : rbView)
	{
		auto& rb = m_Scene.GetComponent<RigidbodyComponent>(e);
		if (!rb.RuntimeBodyID.IsInvalid()) continue;

		auto& transform = m_Scene.GetComponent<TransformComponent>(e);
		JPH::BodyCreationSettings settings = BuildBodyCreationSettings(rb, transform);

		Physics::UserData* data = new Physics::UserData{};
		data->filter = rb.collisionFilter;
		data->entity = e;
		settings.mUserData =(JPH::uint64)data;

		JPH::ShapeRefC shape = m_ShapeBuilder.BuildShape(m_Scene.GetRegistry(), e);
		settings.SetShape(shape);

		rb.RuntimeBodyID = BodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);
		rb.currentPosition = math::FromJoltRVec3(BodyInterface().GetPosition(rb.RuntimeBodyID));
		rb.currentRotation = math::FromJoltQuat(BodyInterface().GetRotation(rb.RuntimeBodyID));
		rb.previousPosition = rb.currentPosition;
		rb.previousRotation = rb.currentRotation;
		rb.interpolationReady = true;

	}
}

void PhysicsSystem::BuildCharacterBodies()
{
	auto cbView = m_Scene.GetRegistry().view<CharacterBodyComponent>();
	for (auto& e : cbView)
	{
		auto& cb = m_Scene.GetComponent<CharacterBodyComponent>(e);
		if (cb.character) continue;
		auto& transform = m_Scene.GetComponent<TransformComponent>(e);

		auto settings = BuildCharacterBodyCreationSettings(cb, transform);
		auto shape = m_ShapeBuilder.BuildShape(m_Scene.GetRegistry(), e);
		settings->mShape = shape;
		//settings->mInnerBodyShape = shape;
		cb.character = new JPH::CharacterVirtual(
			settings,
			math::ToJoltVec3(transform.GetWorldPosition()),
			math::ToJoltQuat(transform.GetWorldRotation()),
			(int)e,
			&m_PhysicsSystem
		);
		cb.character->SetCharacterVsCharacterCollision(&m_CharacterVsCharacterCollision);
		m_CharacterVsCharacterCollision.Add(cb.character);

		Physics::UserData* data = new Physics::UserData{};
		data->filter = cb.collisionFilter;
		data->entity = e;
		cb.character->SetUserData((JPH::uint64)data);

		cb.character->SetListener(&m_CharacterContactListener);
		cb.currentPosition = math::FromJoltRVec3(cb.character->GetPosition());
		cb.currentRotation = math::FromJoltQuat(cb.character->GetRotation());
		cb.previousPosition = cb.currentPosition;
		cb.previousRotation = cb.currentRotation;
		cb.interpolationReady = true;

		cb.worldScaleCache = transform.GetWorldScale();
	}
}

void PhysicsSystem::SyncBodiesFromTransforms()
{
	bool isPlayMode = m_Scene.GetState() == SceneState::PLAY;

	auto rbView = m_Scene.GetRegistry().view<RigidbodyComponent, TransformComponent>();
	for (auto e : rbView)
	{
		auto& rb = m_Scene.GetComponent<RigidbodyComponent>(e);
		if (rb.RuntimeBodyID.IsInvalid()) continue;

		if (isPlayMode && rb.bodyType == Physics::MotionType::DYNAMIC)
			continue;

		auto& transform = m_Scene.GetComponent<TransformComponent>(e);

		// if (isPlayMode && !transform.IsDirty()) continue; 

		BodyInterface().SetPositionAndRotation(
			rb.RuntimeBodyID,
			math::ToJoltRVec3(transform.GetWorldPosition()),
			math::ToJoltQuat(transform.GetWorldRotation()),
			JPH::EActivation::Activate
		);
	}

	auto cbView = m_Scene.GetRegistry().view<CharacterBodyComponent, TransformComponent>();
	for (auto e : cbView)
	{
		auto& cb = m_Scene.GetComponent<CharacterBodyComponent>(e);
		if (!cb.character) continue;

		auto& transform = m_Scene.GetComponent<TransformComponent>(e);

		// if (isPlayMode && !transform.IsDirty()) continue;

		cb.character->SetPosition(math::ToJoltRVec3(transform.GetWorldPosition()));
		cb.character->SetRotation(math::ToJoltQuat(transform.GetWorldRotation()));
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
		if (rb.bodyType == Physics::MotionType::STATIC) continue;

		rb.previousPosition = rb.currentPosition;
		rb.previousRotation = rb.currentRotation;

		rb.currentPosition = math::FromJoltRVec3(BodyInterface().GetPosition(rb.RuntimeBodyID));
		rb.currentRotation = math::FromJoltQuat(BodyInterface().GetRotation(rb.RuntimeBodyID));
	}
}

void rv::PhysicsSystem::SyncCharacterTransforms()
{
	auto view = m_Scene.GetRegistry().view<CharacterBodyComponent, TransformComponent>();
	for (auto& e : view)
	{
		auto& cb = view.get<CharacterBodyComponent>(e);
		if (!cb.character) continue;

		cb.previousPosition = cb.currentPosition;
		cb.previousRotation = cb.currentRotation;

		cb.currentPosition = math::FromJoltRVec3(cb.character->GetPosition());
		cb.currentRotation = math::FromJoltQuat(cb.character->GetRotation());
	}
}

JPH::BodyCreationSettings PhysicsSystem::BuildBodyCreationSettings(RigidbodyComponent& rbComp, TransformComponent& tComp)
{
	JPH::BodyCreationSettings settings;
	JPH::EMotionType motionType = JPH::EMotionType::Dynamic;
	JPH::ObjectLayer layer = Physics::Layers::MOVING;

	if (rbComp.bodyType == Physics::MotionType::STATIC) {
		motionType = JPH::EMotionType::Static;
		layer = Physics::Layers::NON_MOVING;
	}
	else if (rbComp.bodyType == Physics::MotionType::KINEMATIC) {
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
	//TODO: override both mass and innertia for bodies with mesh colliders

	settings.mMaxLinearVelocity = rbComp.maxLinearVelocity;
	settings.mMaxAngularVelocity = rbComp.maxAngularVelocity;

	settings.mAllowSleeping = rbComp.allowSleep;

	if (!rbComp.isSensor)
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
		updateSettings.mStickToFloorStepDown = { 0.0f,-0.3,0.0 };
		updateSettings.mWalkStairsStepUp = { 0.0f,0.2,0.0 };

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

void PhysicsSystem::OnRigidbodyDestroyed(entt::registry& reg, entt::entity e)
{
	auto& rb = reg.get<RigidbodyComponent>(e);
	if (rb.RuntimeBodyID.IsInvalid()) return;

	Physics::UserData* data = reinterpret_cast<Physics::UserData*>(BodyInterface().GetUserData(rb.RuntimeBodyID));
	delete data;

	BodyInterface().RemoveBody(rb.RuntimeBodyID);
	BodyInterface().DestroyBody(rb.RuntimeBodyID);
}

void PhysicsSystem::OnCharacterBodyDestroyed(entt::registry& reg, entt::entity e)
{
	auto& cb = reg.get<CharacterBodyComponent>(e);
	if (!cb.character) return;

	Physics::UserData* data = reinterpret_cast<Physics::UserData*>(cb.character->GetUserData());
	delete data;

	cb.character = nullptr;
}

void PhysicsSystem::OnShapeChanged(entt::registry& reg, entt::entity e)
{
	if (auto* rb = reg.try_get<RigidbodyComponent>(e))
		rb->SetShapeDirty();

	if (auto* cb = reg.try_get<CharacterBodyComponent>(e))
		cb->SetShapeDirty();
}

bool PhysicsSystem::IsAnyColliderDirty(entt::entity e)
{
	auto& reg = m_Scene.GetRegistry();
	if (auto* c = reg.try_get<BoxColliderComponent>(e))        if (c->IsDirty()) return true;
	if (auto* c = reg.try_get<SphereColliderComponent>(e))     if (c->IsDirty()) return true;
	if (auto* c = reg.try_get<CapsuleColliderComponent>(e))    if (c->IsDirty()) return true;
	if (auto* c = reg.try_get<CylinderColliderComponent>(e))   if (c->IsDirty()) return true;
	if (auto* c = reg.try_get<MeshColliderComponent>(e))       if (c->IsDirty()) return true;
	if (auto* c = reg.try_get<ConvexHullColliderComponent>(e)) if (c->IsDirty()) return true;
	return false;
}

void PhysicsSystem::ClearColliderDirtyFlags(entt::entity e)
{
	auto& reg = m_Scene.GetRegistry();
	if (auto* c = reg.try_get<BoxColliderComponent>(e))			 c->ClearDirty();
	if (auto* c = reg.try_get<SphereColliderComponent>(e))		 c->ClearDirty();
	if (auto* c = reg.try_get<CapsuleColliderComponent>(e))		 c->ClearDirty();
	if (auto* c = reg.try_get<CylinderColliderComponent>(e))	 c->ClearDirty();
	if (auto* c = reg.try_get<MeshColliderComponent>(e))		 c->ClearDirty();
	if (auto* c = reg.try_get<ConvexHullColliderComponent>(e))	 c->ClearDirty();
}

void PhysicsSystem::CheckAndUpdateScaleDirty(entt::entity e, glm::vec3& scaleCache,bool isCharacter)
{
	const glm::vec3 worldScale = m_Scene.GetRegistry().get<TransformComponent>(e).GetWorldScale();
	if (glm::all(glm::epsilonEqual(worldScale, scaleCache, 0.0001f))) return;

	scaleCache = worldScale;

	if (isCharacter)
	{
		auto& cb = m_Scene.GetRegistry().get<CharacterBodyComponent>(e);
		cb.SetShapeDirty();
	}
	else
	{
		auto& rb = m_Scene.GetRegistry().get<RigidbodyComponent>(e);
		rb.SetShapeDirty();
	}
}
