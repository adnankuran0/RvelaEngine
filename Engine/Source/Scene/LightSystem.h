#pragma once
#include "Renderer/Light.h"

namespace rv {

class Scene;

class LightSystem
{
public:
	LightSystem(Scene& scene) : m_Scene(scene) {}
	std::vector<PointLight> CollectPointLights() noexcept;
	std::optional<DirectionalLight> CollectDirectionalLight(glm::vec3 cameraPos) noexcept;

private:
	Scene& m_Scene;

};

}