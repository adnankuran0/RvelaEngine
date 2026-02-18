#pragma once
#include "Rendering/Renderer.h"

namespace rv {

class Scene;
class ICamera;

enum class RenderProfile
{
	RUNTIME,
	EDITOR
};

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
	RenderProfile profile = RenderProfile::EDITOR;

	void Clear()
	{
		camera = nullptr;
		pointLights.clear();
		directionalLight.reset();
		viewportWidth = 1920;
		viewportHeight = 1080;
		scene = nullptr;
	}

	
};

}