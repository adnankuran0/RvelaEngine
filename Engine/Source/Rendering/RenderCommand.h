#pragma once
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/MaterialComponent.h"

namespace rv { 

struct RenderCommand
{
public:
	RenderCommand(const TransformComponent& t, const MeshRendererComponent& m, MaterialComponent& mat, bool isSelected)
		: transform(t), mesh(m), material(mat), isSelected(isSelected) {
	}
	const TransformComponent& transform;
	const MeshRendererComponent& mesh;
	MaterialComponent& material;
	const bool isSelected;
};

}