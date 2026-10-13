#pragma once
#include "Scene/Scene.h"
#include <entt/entt.h>

namespace rv {

class TransformInspectors
{
public:
	static void DrawTransform(Scene& scene, entt::entity selectedEntity);
	static void DrawHierarchy(Scene& scene, entt::entity selectedEntity);
	static void Draw(Scene& scene, entt::entity selectedEntity);
};

} // namespace rv
