#pragma once
#include "entt/entt.h"

namespace rv::Physics {

struct RaycastResult
{
	entt::entity entity = entt::null;
	bool hit = false;
	float distance = 0.0f;
	glm::vec3 point = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);
};

}