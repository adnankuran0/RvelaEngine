#pragma once
#include "Light.h"

namespace rv {

class Scene;
class Camera;
class Environment;

enum class RenderProfile
{
	RUNTIME,
	EDITOR
};


struct RenderContext
{
public:
	RenderContext() = default;
	Camera* camera;
	std::vector<PointLight> pointLights;
	std::optional<DirectionalLight> directionalLight;
	unsigned int viewportWidth;
	unsigned int viewportHeight;
	Scene* scene;
	RenderProfile profile = RenderProfile::EDITOR;
	Environment* environment;

	void Clear()
	{
		camera = nullptr;
		pointLights.clear();
		directionalLight.reset();
		scene = nullptr;
		environment = nullptr;
	}

	
};

}