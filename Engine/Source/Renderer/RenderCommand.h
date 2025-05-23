#pragma once
#include "../Scene/Components/TransformComponent.h"
#include "../Scene/Components/MeshRendererComponent.h"
#include "../Scene/Components/MaterialComponent.h"

struct RenderCommand
{
public:
	RenderCommand(TransformComponent& t, MeshRendererComponent& m, MaterialComponent& mat)
		: transform(t), mesh(m), material(mat) {
	}
	TransformComponent& transform;
	MeshRendererComponent& mesh;
	MaterialComponent& material;
};