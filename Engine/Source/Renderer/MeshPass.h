#pragma once
#include "RenderPass.h"
#include "Scene/Components.h"
#include "Renderer.h"
#include "EditorCamera.h"


class MeshPass : public RenderPass
{
public:
	void SetCamera(EditorCamera* cam)
	{
		camera = cam;
	}

	void SetLights(const std::vector<PointLightData>& pLights, const DirectionalLightData* dLight) {
		pointLights = pLights;
		directionalLight = dLight;
	}

	void AddRenderable(WorldTransformComponent* transform, MeshRendererComponent* mesh, MaterialComponent* material) {
		renderables.emplace_back(transform, mesh, material);
	}

	void Execute() override;

private:
	EditorCamera* camera = nullptr;
	std::vector<PointLightData> pointLights;
	const DirectionalLightData* directionalLight = nullptr;

	std::vector<std::tuple<WorldTransformComponent*, MeshRendererComponent*, MaterialComponent*>> renderables;
};

