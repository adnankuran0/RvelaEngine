#pragma once
#include "Renderer/Light.h"
#include <array>

namespace rv {

class Scene;
class Camera;

class LightSystem
{
public:
	LightSystem(Scene& scene) : m_Scene(scene) {}
	std::vector<PointLight> CollectPointLights() noexcept;
	std::vector<SpotLight> CollectSpotLights() noexcept;
	std::optional<DirectionalLight> CollectDirectionalLight(Camera* camera) noexcept;
	std::optional<DirectionalLight> CollectDirectionalLight(glm::vec3 cameraPos) noexcept;

private:
	Scene& m_Scene;
	uint32_t m_FrameCounter = 0;
	bool m_HasCachedCascades = false;
	uint32_t m_CachedDirLightID = 0;
	glm::vec3 m_CachedDirLightDir{ 0.0f };
	std::array<glm::mat4, NUM_SHADOW_CASCADES> m_CachedCascadeMatrices{};
};

}