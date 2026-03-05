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
		entt::registry& reg;
		entt::entity e;
		TransformComponent& tc;
		MeshComponent* mc;
		glm::vec3 worldScale;
		bool isScaleUniform;
		JPH::StaticCompoundShapeSettings& compound;
	};

	int TryAddBoxShape(ShapeBuildParams& params);
	int TryAddSphereShape(ShapeBuildParams& params);
	int TryAddCapsuleShape(ShapeBuildParams& params);
	int TryAddCylinderShape(ShapeBuildParams& params);
	int TryAddMeshShape(ShapeBuildParams& params);
	int TryAddConvexHullShape(ShapeBuildParams& params);
};

}
