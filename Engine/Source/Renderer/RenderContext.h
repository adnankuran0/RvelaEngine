#pragma once
#include "../Renderer/Camera.h"
#include"../Scene/Scene.h"

struct RenderContext
{
public:
	RenderContext() = default;
	EditorCamera* camera;
	std::vector<PointLightData> pointLights;
	std::optional<DirectionalLightData> directionalLight;

	bool IsValid()
	{
		if (!camera)
		{
			return false;
		}
		return true;
	}
};
