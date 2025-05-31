#pragma once
#include "../Scene/Components/TransformComponent.h"
#include "../Scene/Components/MeshRendererComponent.h"
#include "../Scene/Components/MaterialComponent.h"

struct RenderCommand
{
public:
	RenderCommand(const TransformComponent& t, const MeshRendererComponent& m, const MaterialComponent& mat)
		: transform(t), mesh(m), material(mat) {
	}
	const TransformComponent& transform;
	const MeshRendererComponent& mesh;
	const MaterialComponent& material;
};