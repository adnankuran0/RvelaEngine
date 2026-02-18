#pragma once
#include "entt/entt.h"
#include "glm/glm.hpp"

namespace rv {

class Engine;

class Viewport
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity);
};

}