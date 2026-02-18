#pragma once
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/MaterialComponent.h"

namespace rv { 

struct RenderCommand
{
public:
	RenderCommand(const TransformComponent& t, const MeshRendererComponent& m, MaterialComponent& mat, entt::entity entity)
		: transform(t), mesh(m), material(mat), isSelected(isSelected), entityID(entity) {
	}
	entt::entity entityID;
	const TransformComponent& transform;
	const MeshRendererComponent& mesh;
	MaterialComponent& material;
	const bool isSelected;
};

}