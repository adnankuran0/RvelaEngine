#include "rvelapch.h"
#include "LightSystem.h"
#include "Scene/Scene.h"
#include "Renderer/RenderTypes.h"
#include "Renderer/Camera.h"
#include <limits>
#include <cmath>

using namespace rv;

static std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
	const auto inv = glm::inverse(proj * view);
	std::vector<glm::vec4> frustumCorners;
	frustumCorners.reserve(8);
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const glm::vec4 pt = inv * glm::vec4(
					2.0f * static_cast<float>(x) - 1.0f,
					2.0f * static_cast<float>(y) - 1.0f,
					2.0f * static_cast<float>(z) - 1.0f,
					1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}
	return frustumCorners;
}

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
		data.normalBias = light.shadowBias * 0.1f;
		data.reverseCullFace = light.reverseCullFace;
		data.blurRadius = light.blurRadius;
		lights.push_back(data);
	}
	return lights;
}

std::optional<DirectionalLight> LightSystem::CollectDirectionalLight(Camera* camera) noexcept
{
	auto view = m_Scene.GetRegistry().view<DirectionalLightComponent, TransformComponent>();
	for (auto e : view) {
		if (!m_Scene.IsEntityActive(e))
			continue;

		auto& light = m_Scene.GetComponent<DirectionalLightComponent>(e);
		auto& t = m_Scene.GetComponent<TransformComponent>(e);
		DirectionalLight data;
		data.direction = glm::normalize(t.GetForward());
		data.color = light.color;
		data.intensity = light.intensity;
		data.shadowBias = light.shadowBias;
		data.normalBias = light.normalBias;
		data.castShadows = light.castShadows;
		data.reverseCullFace = light.reverseCullFace;
		data.blurRadius = light.blurRadius;

		float nearClip = camera ? camera->NearClip : 0.1f;
		float farClip = camera ? camera->FarClip : 1000.0f;
		float shadowDistance = std::min(farClip, 150.0f);

		constexpr float lambda = 0.85f;
		for (int i = 0; i < NUM_SHADOW_CASCADES; ++i)
		{
			float p = static_cast<float>(i + 1) / static_cast<float>(NUM_SHADOW_CASCADES);
			float logSplit = nearClip * std::pow(shadowDistance / nearClip, p);
			float uniformSplit = nearClip + (shadowDistance - nearClip) * p;
			data.cascadeSplits[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
		}

		glm::vec3 lightDir = data.direction;
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (std::abs(glm::dot(up, lightDir)) > 0.99f)
			up = glm::vec3(0.0f, 0.0f, 1.0f);

		for (int i = 0; i < NUM_SHADOW_CASCADES; ++i)
		{
			float prevSplit = (i == 0) ? nearClip : data.cascadeSplits[i - 1];
			float currSplit = data.cascadeSplits[i];

			glm::mat4 cascadeProj;
			if (camera && camera->ProjectionType == Camera::Projection::Perspective)
			{
				cascadeProj = glm::perspective(
					glm::radians(camera->FOV),
					camera->GetAspectRatio(),
					prevSplit,
					currSplit
				);
			}
			else if (camera)
			{
				float orthoSize = camera->OrthoSize;
				float aspect = camera->GetAspectRatio();
				cascadeProj = glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, prevSplit, currSplit);
			}
			else
			{
				cascadeProj = glm::perspective(glm::radians(90.0f), 16.0f / 9.0f, prevSplit, currSplit);
			}

			glm::mat4 viewMatrix = camera ? camera->GetViewMatrix() : glm::mat4(1.0f);
			auto corners = GetFrustumCornersWorldSpace(cascadeProj, viewMatrix);

			glm::vec3 center(0.0f);
			for (const auto& v : corners)
				center += glm::vec3(v);
			center /= static_cast<float>(corners.size());

			float radius = 0.0f;
			for (const auto& v : corners)
			{
				float dist = glm::length(glm::vec3(v) - center);
				radius = std::max(radius, dist);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			constexpr float shadowMapResolution = 2048.0f;
			float worldUnitsPerTexel = (2.0f * radius) / shadowMapResolution;
			radius = std::ceil(radius / worldUnitsPerTexel) * worldUnitsPerTexel;
			worldUnitsPerTexel = (2.0f * radius) / shadowMapResolution;

			glm::mat4 tempLightView = glm::lookAt(-lightDir * 100.0f, glm::vec3(0.0f), up);
			glm::vec3 centerLightSpace = glm::vec3(tempLightView * glm::vec4(center, 1.0f));

			centerLightSpace.x = std::floor(centerLightSpace.x / worldUnitsPerTexel) * worldUnitsPerTexel;
			centerLightSpace.y = std::floor(centerLightSpace.y / worldUnitsPerTexel) * worldUnitsPerTexel;

			glm::vec3 snappedCenter = glm::vec3(glm::inverse(tempLightView) * glm::vec4(centerLightSpace, 1.0f));

			float eyeDistance = radius * 2.0f + 50.0f;
			glm::mat4 lightView = glm::lookAt(snappedCenter - lightDir * eyeDistance, snappedCenter, up);

			float minX = -radius;
			float maxX =  radius;
			float minY = -radius;
			float maxY =  radius;

			float zNear = 0.1f;
			float zFar = eyeDistance + radius + 100.0f;

			glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, zNear, zFar);

			data.cascadeMatrices[i] = lightProj * lightView;
		}

		data.lightSpace = data.cascadeMatrices[0];

		return data;
	}
	return std::nullopt;
}

std::optional<DirectionalLight> LightSystem::CollectDirectionalLight(glm::vec3 cameraPos) noexcept
{
	return CollectDirectionalLight(nullptr);
}
