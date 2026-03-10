#pragma once
#include "glm/glm.hpp"

namespace rv::Physics {

struct Collision
{
	glm::vec3 point;
	glm::vec3 normal;
};

}