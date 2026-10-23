#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "entt/entt.h"

namespace rv::Physics {

struct RaycastResult
{
	entt::entity entity = entt::null;
	bool hit = false;
	float distance = 0.0f;
	float fraction = 0.0f;
	glm::vec3 point = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);
	uint32_t bodyId = 0xFFFFFFFF;
};

using ShapeCastResult = RaycastResult;

struct RaycastOptions
{
	glm::vec3 origin = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
	float maxDistance = 1000.0f;
	bool hitInside = false;
	std::vector<entt::entity> ignoredEntities;
	uint32_t layerMask = 0xFFFFFFFF;
};

struct ShapeCastOptions
{
	glm::vec3 start = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
	float maxDistance = 1000.0f;
	bool hitInside = false;
	std::vector<entt::entity> ignoredEntities;
	uint32_t layerMask = 0xFFFFFFFF;
};

}
