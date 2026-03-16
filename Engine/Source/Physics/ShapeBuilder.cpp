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
#include <Core/Ref.h>

using namespace rv::Physics;

JPH::ShapeRefC ShapeBuilder::BuildShape(entt::registry& registry, entt::entity e)
{
	JPH::StaticCompoundShapeSettings compound; // TODO: do not use compound shape for single shapes

	ShapeBuildParams params;
	params.registry = &registry;
	params.transformComponent = &registry.get<TransformComponent>(e);
	params.meshComponent = registry.try_get<MeshComponent>(e);
	params.compoundSettings = &compound;
	params.entity = e;

	const glm::vec3 worldScale = params.transformComponent->GetWorldScale();

	size_t colliderCount = 0;
	colliderCount += TryAddBoxShape(params);
	colliderCount += TryAddSphereShape(params);
	colliderCount += TryAddCapsuleShape(params);
	colliderCount += TryAddCylinderShape(params);
	// TODO: cache shapes per MeshAsset to avoid duplicates
	colliderCount += TryAddMeshShape(params); 
	colliderCount += TryAddConvexHullShape(params);

	if (colliderCount == 0)
	{
		LOG_WARN("Entity with rigidbody has no colliders.");
		JPH::EmptyShapeSettings settings;
		return settings.Create().Get();
	}

	JPH::ShapeRefC compoundShape = compound.Create().Get();
	return TryApplyScale(compoundShape, worldScale);
	
}

bool ShapeBuilder::TryAddBoxShape(ShapeBuildParams& params)
{
	if (auto* box = params.registry->try_get<BoxColliderComponent>(params.entity))
	{
		JPH::BoxShapeSettings boxSettings(math::ToJoltVec3(box->GetSize()));
		JPH::ShapeRefC shape = boxSettings.Create().Get();
		params.compoundSettings->AddShape(math::ToJoltVec3(box->GetOffset()), JPH::Quat::sIdentity(), shape);
		return true;
	}
	return false;
}

bool ShapeBuilder::TryAddSphereShape(ShapeBuildParams& params)
{
	if (auto* sphere = params.registry->try_get<SphereColliderComponent>(params.entity))
	{
		JPH::SphereShapeSettings sphereSettings(sphere->GetRadius());
		JPH::ShapeRefC shape = sphereSettings.Create().Get();
		params.compoundSettings->AddShape(math::ToJoltVec3(sphere->GetOffset()), JPH::Quat::sIdentity(), shape);
		return true;
	}
	return false;
}

bool ShapeBuilder::TryAddCapsuleShape(ShapeBuildParams& params)
{
	if (auto* capsule = params.registry->try_get<CapsuleColliderComponent>(params.entity))
	{
		JPH::CapsuleShapeSettings capsuleSettings(capsule->GetHalfHeight(), capsule->GetRadius());
		JPH::ShapeRefC shape = capsuleSettings.Create().Get();
		params.compoundSettings->AddShape(math::ToJoltVec3(capsule->GetOffset()), JPH::Quat::sIdentity(), shape);
		return true;
	}
	return false;
}

bool ShapeBuilder::TryAddCylinderShape(ShapeBuildParams& params)
{
	if (auto* cylinder = params.registry->try_get<CylinderColliderComponent>(params.entity))
	{
		JPH::CylinderShapeSettings cylinderSettings(cylinder->GetHalfHeight(), cylinder->GetRadius());
		JPH::ShapeRefC shape = cylinderSettings.Create().Get();
		params.compoundSettings->AddShape(math::ToJoltVec3(cylinder->GetOffset()), JPH::Quat::sIdentity(), shape);
		return true;
	}
	return false;
}

bool ShapeBuilder::TryAddMeshShape(ShapeBuildParams& params)
{
	if (auto* meshCollider = params.registry->try_get<MeshColliderComponent>(params.entity))
	{
		if (!params.meshComponent) return false;
		Ref<MeshAsset> mesh = params.meshComponent->GetMesh();

		JPH::Array<JPH::Float3> joltVertices;
		joltVertices.reserve(mesh->GetVertices().size());

		for (const auto& v : mesh->GetVertices())
		{
			joltVertices.push_back(JPH::Float3(v.position.x, v.position.y, v.position.z));
		}

		JPH::Array<JPH::IndexedTriangle> joltTriangles;
		joltTriangles.reserve(mesh->GetIndices().size() / 3);

		for (size_t i = 0; i < mesh->GetIndices().size(); i += 3)
		{
			joltTriangles.push_back(JPH::IndexedTriangle(
				mesh->GetIndices()[i],
				mesh->GetIndices()[i + 1],
				mesh->GetIndices()[i + 2]
			));
		}

		JPH::MeshShapeSettings meshSettings;
		meshSettings.mTriangleVertices = std::move(joltVertices);
		meshSettings.mIndexedTriangles = std::move(joltTriangles);
		meshSettings.mActiveEdgeCosThresholdAngle = meshCollider->GetActiveEdgeTresholdAngle();
		meshSettings.mMaxTrianglesPerLeaf = meshCollider->GetMaxTrianglesPerLeaf();

		JPH::ShapeRefC shape = meshSettings.Create().Get();
		params.compoundSettings->AddShape(math::ToJoltVec3(meshCollider->GetOffset()), JPH::Quat::sIdentity(), shape);
		return true;
	}
	return false;
}

bool ShapeBuilder::TryAddConvexHullShape(ShapeBuildParams& params)
{

	if (auto* convexHullCollider = params.registry->try_get<ConvexHullColliderComponent>(params.entity))
	{
		if (!params.meshComponent) return false;
		Ref<MeshAsset> mesh = params.meshComponent->GetMesh();

		JPH::Array<JPH::Vec3> points;
		points.reserve(mesh->GetVertices().size());

		for (const auto& v : mesh->GetVertices())
		{
			points.push_back(math::ToJoltVec3(v.position));
		}

		JPH::ConvexHullShapeSettings convexHullSettings(points);
		convexHullSettings.mMaxConvexRadius = convexHullCollider->GetMaxConvexRadius();
		JPH::ShapeRefC shape = convexHullSettings.Create().Get();
		params.compoundSettings->AddShape(math::ToJoltVec3(convexHullCollider->GetOffset()), JPH::Quat::sIdentity(), shape);
		return true;
	}
	return false;
}

JPH::ShapeRefC ShapeBuilder::TryApplyScale(JPH::ShapeRefC shape, const glm::vec3& scale)
{
	static constexpr glm::vec3 identity(1.0f);
	if (glm::all(glm::epsilonEqual(scale, identity, 0.0001f)))
		return shape;

	JPH::Shape::ShapeResult shapeResult = shape->ScaleShape(math::ToJoltVec3(scale));

	return shapeResult.IsValid() ? shapeResult.Get() : shape;

}
