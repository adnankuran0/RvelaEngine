#include "rvelapch.h"
#include "ShapeBuilder.h"
#include <Scene/Components/TransformComponent.h>
#include <Scene/Components/MeshComponent.h>
#include "Scene/Components/ColliderComponents.h"
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

using namespace rv::Physics;

JPH::ShapeRefC ShapeBuilder::BuildShape(entt::registry& registry, entt::entity e)
{
	auto& tc = registry.get<TransformComponent>(e);
	glm::vec3 worldScale = tc.GetWorldScale();
	bool isScaleUniform = glm::epsilonEqual(worldScale.x, worldScale.y, 0.0001f) &&
		glm::epsilonEqual(worldScale.y, worldScale.z, 0.0001f);
	auto* meshComp = registry.try_get<MeshComponent>(e);
	JPH::StaticCompoundShapeSettings compound;

	ShapeBuildParams params{ registry,e,tc,meshComp,worldScale,isScaleUniform,compound };
	size_t colliderCount = 0;
	colliderCount += TryAddBoxShape(params);
	colliderCount += TryAddSphereShape(params);
	colliderCount += TryAddCapsuleShape(params);
	colliderCount += TryAddCylinderShape(params);
	colliderCount += TryAddMeshShape(params);
	colliderCount += TryAddConvexHullShape(params);

	if (colliderCount == 0)
	{
		LOG_WARN("Entity with rigidbody has no colliders.");
		JPH::EmptyShapeSettings settings;
		return settings.Create().Get();
	}

	return compound.Create().Get();
}

int ShapeBuilder::TryAddBoxShape(ShapeBuildParams& params)
{
	if (auto* box = params.reg.try_get<BoxColliderComponent>(params.e))
	{
		JPH::BoxShapeSettings boxSettings(math::ToJoltVec3(box->size * params.tc.GetWorldScale()));
		JPH::ShapeRefC shape = boxSettings.Create().Get();
		params.compound.AddShape(math::ToJoltVec3(box->offset * params.tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		return 1;
	}
	return 0;
}

int ShapeBuilder::TryAddSphereShape(ShapeBuildParams& params)
{
	if (auto* sphere = params.reg.try_get<SphereColliderComponent>(params.e))
	{
		float scaledRadius = params.isScaleUniform ? sphere->radius * params.worldScale.x : sphere->radius;
		JPH::SphereShapeSettings sphereSettings(scaledRadius);
		JPH::ShapeRefC shape = sphereSettings.Create().Get();
		params.compound.AddShape(math::ToJoltVec3(sphere->offset), JPH::Quat::sIdentity(), shape);
		return 1;
	}
	return 0;
}

int ShapeBuilder::TryAddCapsuleShape(ShapeBuildParams& params)
{
	if (auto* capsule = params.reg.try_get<CapsuleColliderComponent>(params.e))
	{
		float scaledRadius = params.isScaleUniform ? capsule->radius * params.worldScale.x : capsule->radius;
		float scaledHeight = params.isScaleUniform ? capsule->halfHeight * params.worldScale.y : capsule->halfHeight;

		JPH::CapsuleShapeSettings capsuleSettings(scaledHeight, scaledRadius);
		JPH::ShapeRefC shape = capsuleSettings.Create().Get();
		params.compound.AddShape(math::ToJoltVec3(capsule->offset * params.tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		return 1;
	}
	return 0;
}

int ShapeBuilder::TryAddCylinderShape(ShapeBuildParams& params)
{
	if (auto* cylinder = params.reg.try_get<CylinderColliderComponent>(params.e))
	{
		float scaledRadius = params.isScaleUniform ? cylinder->radius * params.worldScale.x : cylinder->radius;
		float scaledHeight = params.isScaleUniform ? cylinder->halfHeight * params.worldScale.y : cylinder->halfHeight;

		JPH::CylinderShapeSettings cylinderSettings(scaledHeight, scaledRadius);
		JPH::ShapeRefC shape = cylinderSettings.Create().Get();
		params.compound.AddShape(math::ToJoltVec3(cylinder->offset * params.tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		return 1;
	}
	return 0;
}

int ShapeBuilder::TryAddMeshShape(ShapeBuildParams& params)
{
	if (auto* meshCollider = params.reg.try_get<MeshColliderComponent>(params.e))
	{
		if (!params.mc) return 0;

		Ref<MeshAsset> mesh = params.mc->GetMesh();

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
		params.compound.AddShape(math::ToJoltVec3(meshCollider->offset * params.tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		return 1;
	}
	return 0;
}

int ShapeBuilder::TryAddConvexHullShape(ShapeBuildParams& params)
{

	if (auto* convexHullCollider = params.reg.try_get<ConvexHullColliderComponent>(params.e))
	{
		if (!params.mc) return 0;
		Ref<MeshAsset> mesh = params.mc->GetMesh();

		JPH::Array<JPH::Vec3> points;
		points.reserve(mesh->vertices.size());

		for (const auto& v : mesh->vertices)
		{
			points.push_back(math::ToJoltVec3(v.position * params.worldScale));
		}

		JPH::ConvexHullShapeSettings convexHullSettings(points);
		convexHullSettings.mMaxConvexRadius = convexHullCollider->maxConvexRadius;
		JPH::ShapeRefC shape = convexHullSettings.Create().Get();
		params.compound.AddShape(math::ToJoltVec3(convexHullCollider->offset * params.tc.GetWorldScale()), JPH::Quat::sIdentity(), shape);
		return 1;
	}
	return 0;
}