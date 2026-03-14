#pragma once
#include "Jolt/Jolt.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include "entt/entt.h"

namespace rv {
struct TransformComponent;
struct MeshComponent;
}

namespace rv::Physics {

class ShapeBuilder
{
public:
	JPH::ShapeRefC BuildShape(entt::registry& registry, entt::entity e);
private:
	struct ShapeBuildParams
	{
		entt::registry* registry;
		entt::entity entity;
		TransformComponent* transformComponent;
		MeshComponent* meshComponent;
		JPH::StaticCompoundShapeSettings* compoundSettings;
	};

	bool TryAddBoxShape(ShapeBuildParams& params);
	bool TryAddSphereShape(ShapeBuildParams& params);
	bool TryAddCapsuleShape(ShapeBuildParams& params);
	bool TryAddCylinderShape(ShapeBuildParams& params);
	bool TryAddMeshShape(ShapeBuildParams& params);
	bool TryAddConvexHullShape(ShapeBuildParams& params);

	JPH::ShapeRefC TryApplyScale(JPH::ShapeRefC shape, const glm::vec3& scale);
};

}
