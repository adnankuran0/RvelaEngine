#pragma once
#include "Scene/Scene.h"
#include <entt/entt.h>

namespace rv {

class PhysicsInspectors
{
public:
	static void Draw(Scene& scene, entt::entity selectedEntity);
};

} // namespace rv
