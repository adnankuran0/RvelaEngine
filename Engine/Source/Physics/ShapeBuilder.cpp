#include "rvelapch.h"
#include "ShapeBuilder.h"
#include <Scene/Components/TransformComponent.h>
#include <Scene/Components/MeshComponent.h>
#include "Scene/Components/ColliderComponents.h"
#include "Scene/Components/SceneTreeComponent.h"
#include "Scene/Components/TagComponent.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Core/Ref.h>
#include "Math/RvelaMath.h"
#include <functional>

using namespace rv::Physics;

JPH::ShapeRefC ShapeBuilder::BuildShape(entt::registry& registry, entt::entity e)
{
	JPH::StaticCompoundShapeSettings compound;

	auto& rootTransform = registry.get<TransformComponent>(e);
	const glm::vec3 rootWorldPos = rootTransform.GetWorldPosition();
	const glm::quat rootWorldRot = rootTransform.GetWorldRotation();
	const glm::quat invRootRot = glm::inverse(rootWorldRot);

	size_t colliderCount = 0;

	std::function<void(entt::entity)> traverse = [&](entt::entity curr)
	{
		if (curr == entt::null || !registry.valid(curr))
			return;

		if (auto* tag = registry.try_get<TagComponent>(curr))
		{
			if (!tag->isActive)
				return;
		}

		auto& currTransform = registry.get<TransformComponent>(curr);
		const glm::vec3 currWorldPos = currTransform.GetWorldPosition();
		const glm::quat currWorldRot = currTransform.GetWorldRotation();
		const glm::vec3 currWorldScale = currTransform.GetWorldScale();

		ShapeBuildParams params;
		params.registry = &registry;
		params.entity = curr;
		params.meshComponent = registry.try_get<MeshComponent>(curr);
		params.compoundSettings = &compound;
		params.relPos = invRootRot * (currWorldPos - rootWorldPos);
		params.relRot = invRootRot * currWorldRot;
		params.relScale = currWorldScale;

		colliderCount += TryAddBoxShape(params);
		colliderCount += TryAddSphereShape(params);
		colliderCount += TryAddCapsuleShape(params);
		colliderCount += TryAddCylinderShape(params);
		// TODO: cache shapes per MeshAsset to avoid duplicates
		colliderCount += TryAddMeshShape(params);
		colliderCount += TryAddConvexHullShape(params);

		if (registry.any_of<SceneTreeComponent>(curr))
		{
			const auto& tree = registry.get<SceneTreeComponent>(curr);
			for (entt::entity child : tree.children)
			{
				if (child == entt::null || !registry.valid(child))
					continue;

				// Stop at subbodies
				if (registry.any_of<RigidbodyComponent, CharacterBodyComponent>(child))
					continue;

				traverse(child);
			}
		}
	};

	traverse(e);

	if (colliderCount == 0)
	{
		LOG_WARN("Entity with rigidbody has no colliders.");
		JPH::EmptyShapeSettings settings;
		return settings.Create().Get();
	}

	return compound.Create().Get();
}

bool ShapeBuilder::TryAddBoxShape(ShapeBuildParams& params)
{
	if (auto* box = params.registry->try_get<BoxColliderComponent>(params.entity))
	{
		glm::vec3 scaledSize = glm::max(glm::abs(box->GetSize() * params.relScale), glm::vec3(0.001f));
		JPH::BoxShapeSettings boxSettings(math::ToJoltVec3(scaledSize));
		JPH::ShapeRefC shape = boxSettings.Create().Get();

		glm::vec3 posInBody = params.relPos + params.relRot * (params.relScale * box->GetOffset());
		glm::quat rotInBody = params.relRot;

		params.compoundSettings->AddShape(math::ToJoltVec3(posInBody), math::ToJoltQuat(rotInBody), shape, (JPH::uint32)params.entity);
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
		shape = TryApplyScale(shape, params.relScale);

		glm::vec3 posInBody = params.relPos + params.relRot * (params.relScale * sphere->GetOffset());
		glm::quat rotInBody = params.relRot;

		params.compoundSettings->AddShape(math::ToJoltVec3(posInBody), math::ToJoltQuat(rotInBody), shape, (JPH::uint32)params.entity);
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
		shape = TryApplyScale(shape, params.relScale);

		glm::vec3 posInBody = params.relPos + params.relRot * (params.relScale * capsule->GetOffset());
		glm::quat rotInBody = params.relRot;

		params.compoundSettings->AddShape(math::ToJoltVec3(posInBody), math::ToJoltQuat(rotInBody), shape, (JPH::uint32)params.entity);
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
		shape = TryApplyScale(shape, params.relScale);

		glm::vec3 posInBody = params.relPos + params.relRot * (params.relScale * cylinder->GetOffset());
		glm::quat rotInBody = params.relRot;

		params.compoundSettings->AddShape(math::ToJoltVec3(posInBody), math::ToJoltQuat(rotInBody), shape, (JPH::uint32)params.entity);
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
		if (!mesh) return false;

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
		shape = TryApplyScale(shape, params.relScale);

		glm::vec3 posInBody = params.relPos + params.relRot * (params.relScale * meshCollider->GetOffset());
		glm::quat rotInBody = params.relRot;

		params.compoundSettings->AddShape(math::ToJoltVec3(posInBody), math::ToJoltQuat(rotInBody), shape, (JPH::uint32)params.entity);
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
		if (!mesh) return false;

		JPH::Array<JPH::Vec3> points;
		points.reserve(mesh->GetVertices().size());

		for (const auto& v : mesh->GetVertices())
		{
			points.push_back(math::ToJoltVec3(v.position));
		}

		JPH::ConvexHullShapeSettings convexHullSettings(points);
		convexHullSettings.mMaxConvexRadius = convexHullCollider->GetMaxConvexRadius();
		JPH::ShapeRefC shape = convexHullSettings.Create().Get();
		shape = TryApplyScale(shape, params.relScale);

		glm::vec3 posInBody = params.relPos + params.relRot * (params.relScale * convexHullCollider->GetOffset());
		glm::quat rotInBody = params.relRot;

		params.compoundSettings->AddShape(math::ToJoltVec3(posInBody), math::ToJoltQuat(rotInBody), shape, (JPH::uint32)params.entity);
		return true;
	}
	return false;
}

JPH::ShapeRefC ShapeBuilder::TryApplyScale(JPH::ShapeRefC shape, const glm::vec3& scale)
{
	if (!shape)
		return shape;

	static constexpr glm::vec3 identity(1.0f);
	if (glm::all(glm::epsilonEqual(scale, identity, 0.0001f)))
		return shape;

	glm::vec3 safeScale = glm::max(glm::abs(scale), glm::vec3(0.001f));
	if (scale.x < 0.0f) safeScale.x = -safeScale.x;
	if (scale.y < 0.0f) safeScale.y = -safeScale.y;
	if (scale.z < 0.0f) safeScale.z = -safeScale.z;

	JPH::Shape::ShapeResult shapeResult = shape->ScaleShape(math::ToJoltVec3(safeScale));

	return shapeResult.IsValid() ? shapeResult.Get() : shape;
}
