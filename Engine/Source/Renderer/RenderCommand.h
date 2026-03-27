#pragma once
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "entt/entt.h"

namespace rv { 

struct RenderCommand
{
public:
	RenderCommand(const TransformComponent& t, const MeshRendererComponent& m, MaterialComponent& mat, entt::entity entity)
		: transform(t), mesh(m), material(mat), entityID(entity) {
	}
	const entt::entity entityID;
	const TransformComponent& transform;
	const MeshRendererComponent& mesh;
	MaterialComponent& material;
};

}