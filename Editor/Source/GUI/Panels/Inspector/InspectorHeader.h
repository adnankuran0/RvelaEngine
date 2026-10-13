#pragma once
#include "Scene/Scene.h"
#include <entt/entt.h>

namespace rv {

class InspectorHeader
{
public:
	// Returns false if entity was reverted/replaced and Inspector drawing should early-out this frame.
	static bool Draw(Scene& scene, entt::entity selectedEntity);
};

} // namespace rv
