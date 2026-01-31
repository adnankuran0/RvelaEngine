#pragma once
#include "Scene/Scene.h"
#include "Scene/ICamera.h"

struct RenderContext
{
public:
	RenderContext() = default;
	ICamera* camera;
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
