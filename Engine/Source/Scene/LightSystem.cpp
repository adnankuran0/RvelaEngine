#include "rvelapch.h"
#include "LightSystem.h"
#include "Scene/Scene.h"
#include "Renderer/RenderTypes.h"

using namespace rv;

std::vector<PointLight> LightSystem::CollectPointLights() noexcept {
	std::vector<PointLight> lights;
	lights.reserve(rv::MaxPointLights);
	int nextShadowIndex = 0;
	auto view = m_Scene.GetRegistry().view<PointLightComponent, TransformComponent>();
	for (auto e : view) {
		if (!m_Scene.IsEntityActive(e))
			continue;
		if (lights.size() >= rv::MaxPointLights)
			break;

		auto& light = m_Scene.GetComponent<PointLightComponent>(e);
		auto& t = m_Scene.GetComponent<TransformComponent>(e);
		PointLight data;
		data.position = t.GetWorldPosition();
		data.color = light.color;
		data.intensity = light.intensity;
		data.radius = light.radius;
		data.falloff = light.falloff;
		data.castShadows = light.castShadows && nextShadowIndex < rv::MaxPointLights;
		data.shadowIndex = data.castShadows ? nextShadowIndex++ : -1;
		data.shadowBias = light.shadowBias;
		data.reverseCullFace = light.reverseCullFace;
		data.blurRadius = light.blurRadius;
		lights.push_back(data);
	}
	return lights;
}

std::optional<DirectionalLight> LightSystem::CollectDirectionalLight(glm::vec3 cameraPos) noexcept
{
	auto view = m_Scene.GetRegistry().view<DirectionalLightComponent, TransformComponent>();
	for (auto e : view) {
		if (!m_Scene.IsEntityActive(e))
			continue;

		auto& light = m_Scene.GetComponent<DirectionalLightComponent>(e);
		auto& t = m_Scene.GetComponent<TransformComponent>(e);
		DirectionalLight data;
		data.direction = t.GetForward();
		data.color = light.color;
		data.intensity = light.intensity;
		data.shadowBias = light.shadowBias;
		data.castShadows = light.castShadows;
		data.reverseCullFace = light.reverseCullFace;
		data.blurRadius = light.blurRadius;

		glm::vec3 lightDir = data.direction;
		glm::vec3 sceneCenter = cameraPos;
		glm::mat4 lightView = glm::lookAt(sceneCenter - lightDir * 50.0f, sceneCenter, glm::vec3(0, 1, 0));
		float orthoSize = 35.0f;
		glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 120.0f);
		data.lightSpace = lightProjection * lightView;

		return data;
	}
	return std::nullopt;
}
