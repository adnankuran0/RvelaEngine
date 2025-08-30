#pragma once
#include "Rendering/Camera.h"
#include "Scene/Scene.h"

struct RenderContext
{
public:
	RenderContext() = default;
	EditorCamera* camera;
	std::vector<PointLightData> pointLights;
	std::optional<DirectionalLightData> directionalLight;
	unsigned int viewportWidth;
	unsigned int viewportHeight;
	Scene* scene;

	bool IsValid()
	{
		if (!camera)
		{
			return false;
		}
		return true;
	}
};
