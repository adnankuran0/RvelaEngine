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
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>

#include "Layers.h"
#include <iostream>
#include <cstdarg>
#include <thread>
#include "Scene/Scene.h"
#include <Scene/Components/RigidbodyComponent.h>
#include <Scene/Components/TransformComponent.h>
#include "Math/RvelaMath.h"
#include <glm/gtx/component_wise.hpp>

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
	InitialiseBodies();
	m_PhysicsSystem.Update(dt, cCollisionSteps, &m_TempAllocator, &m_JobSystem);
	SyncTransforms();
}

void PhysicsSystem::AddForce(RigidbodyComponent* comp, const glm::vec3& force)
{
	if (comp && comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().AddForce(comp->RuntimeBodyID, math::ToJoltVec3(force));
}

void PhysicsSystem::AddImpulse(RigidbodyComponent* comp, const glm::vec3& impulse)
{
	if (comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().AddImpulse(comp->RuntimeBodyID, math::ToJoltVec3(impulse));
}

void PhysicsSystem::SetVelocity(RigidbodyComponent* comp, const glm::vec3& velocity)
{
	if (comp->RuntimeBodyID.IsInvalid()) return;

	BodyInterface().SetLinearVelocity(comp->RuntimeBodyID, math::ToJoltVec3(velocity));
}

glm::vec3 PhysicsSystem::GetVelocity(RigidbodyComponent* comp)
{
	return math::FromJoltVec3(BodyInterface().GetLinearVelocity(comp->RuntimeBodyID));
}

void PhysicsSystem::OnStart()
{
	m_PhysicsSystem.OptimizeBroadPhase();
	InitialiseBodies();
}

void PhysicsSystem::InitialiseBodies()
{
	auto view = m_Scene.GetRegistry().view<RigidbodyComponent>();
	for (auto& e : view)
	{
		auto& rb = m_Scene.GetComponent<RigidbodyComponent>(e);
		if (!rb.RuntimeBodyID.IsInvalid()) continue;

		auto& transform = m_Scene.GetComponent<TransformComponent>(e);
		JPH::BodyCreationSettings settings = BuildBodyCreationSettings(rb, transform);

		JPH::ShapeRefC shape = BuildShape(e);
		settings.SetShape(shape);

		rb.RuntimeBodyID = BodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate);
		
	}
}

void PhysicsSystem::SyncTransforms()
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

JPH::ShapeRefC PhysicsSystem::BuildShape(entt::entity e)
{
	auto& registry = m_Scene.GetRegistry();
	auto& tc = registry.get<TransformComponent>(e);
	glm::vec3 worldScale = tc.GetWorldScale();

	JPH::StaticCompoundShapeSettings compound;

	bool hasCollider = false;

	if (auto* box = registry.try_get<BoxColliderComponent>(e))
	{
		JPH::BoxShapeSettings boxSettings(math::ToJoltVec3(box->size * tc.GetWorldScale()) );
		JPH::ShapeRefC shape = boxSettings.Create().Get();
		compound.AddShape(math::ToJoltVec3(box->offset * tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		hasCollider = true;
	}

	bool isScaleUniform = glm::epsilonEqual(worldScale.x, worldScale.y, 0.0001f) &&
		glm::epsilonEqual(worldScale.y, worldScale.z, 0.0001f);

	if (auto* sphere = registry.try_get<SphereColliderComponent>(e))
	{
		float scaledRadius = isScaleUniform ? sphere->radius * worldScale.x : sphere->radius;
		JPH::SphereShapeSettings sphereSettings(scaledRadius);
		JPH::ShapeRefC shape = sphereSettings.Create().Get();
		compound.AddShape(math::ToJoltVec3(sphere->offset), JPH::Quat::sIdentity(), shape);
		hasCollider = true;
	}

	if (auto* capsule = registry.try_get<CapsuleColliderComponent>(e))
	{
		float scaledRadius = isScaleUniform ? capsule->radius * worldScale.x : capsule->radius;
		float scaledHeight = isScaleUniform ? capsule->halfHeight * worldScale.y : capsule->halfHeight;

		JPH::CapsuleShapeSettings capsuleSettings(scaledHeight, scaledRadius);
		JPH::ShapeRefC shape = capsuleSettings.Create().Get();
		compound.AddShape(math::ToJoltVec3(capsule->offset * tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		hasCollider = true;
	}

	if (auto* cylinder = registry.try_get<CylinderColliderComponent>(e))
	{
		float scaledRadius = isScaleUniform ? cylinder->radius * worldScale.x : cylinder->radius;
		float scaledHeight = isScaleUniform ? cylinder->halfHeight * worldScale.y : cylinder->halfHeight;

		JPH::CylinderShapeSettings cylinderSettings(scaledHeight, scaledRadius);
		JPH::ShapeRefC shape = cylinderSettings.Create().Get();
		compound.AddShape(math::ToJoltVec3(cylinder->offset * tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		hasCollider = true;
	}

	auto* meshCollider = registry.try_get<MeshColliderComponent>(e);
	auto* meshComp = registry.try_get<MeshComponent>(e);
	if (meshCollider && meshComp)
	{

		Ref<MeshAsset> mesh = meshComp->GetMesh();
			
		JPH::Array<JPH::Float3> joltVertices;
		joltVertices.reserve(mesh->vertices.size());

		for (const auto& v : mesh->vertices)
		{
			joltVertices.push_back(JPH::Float3(v.position.x, v.position.y, v.position.z));
		}

		JPH::Array<JPH::IndexedTriangle> joltTriangles;
		joltTriangles.reserve(mesh->indices.size() / 3);

		for (size_t i = 0; i < mesh->indices.size(); i += 3)
		{
			joltTriangles.push_back(JPH::IndexedTriangle(
				mesh->indices[i],
				mesh->indices[i + 1],
				mesh->indices[i + 2]
			));
		}

		JPH::MeshShapeSettings meshSettings;
		meshSettings.mTriangleVertices = joltVertices;
		meshSettings.mIndexedTriangles = joltTriangles;
		meshSettings.mActiveEdgeCosThresholdAngle = meshCollider->activeEdgeTresholdAngle;
		meshSettings.mMaxTrianglesPerLeaf = meshCollider->maxTrianglesPerLeaf;

		JPH::ShapeRefC shape = meshSettings.Create().Get();
		compound.AddShape(math::ToJoltVec3(meshCollider->offset * tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		hasCollider = true;
	}

	auto* convexHullCollider = registry.try_get<ConvexHullColliderComponent>(e);
	if (meshComp && convexHullCollider)
	{
		Ref<MeshAsset> mesh = meshComp->GetMesh();

		JPH::Array<JPH::Vec3> points;
		points.reserve(mesh->vertices.size());

		for (const auto& v : mesh->vertices)
		{
			points.push_back(math::ToJoltVec3(v.position * worldScale));
		}

		JPH::ConvexHullShapeSettings convexHullSettings(points);
		convexHullSettings.mMaxConvexRadius = convexHullCollider->maxConvexRadius;
		JPH::ShapeRefC shape = convexHullSettings.Create().Get();
		compound.AddShape(math::ToJoltVec3(convexHullCollider->offset * tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		hasCollider = true;
	}


	if (!hasCollider)
	{
		LOG_WARN("Entity with rigidbody has no colliders.");
		JPH::EmptyShapeSettings settings;
		return settings.Create().Get();
	}

	return compound.Create().Get();
}
