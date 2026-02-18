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

struct RenderFeatures
{
	bool ssao = true;
	bool ssr = true;
	bool bloom = true;
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
	RenderFeatures renderFeatures;

	void Clear()
	{
		camera = nullptr;
		pointLights.clear();
		directionalLight.reset();
		scene = nullptr;
	}

	
};

}