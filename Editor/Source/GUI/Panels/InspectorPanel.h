#pragma once
#include "entt/entt.h"

#include <vector>

namespace rv {

class Engine;

class InspectorPanel
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity, const std::vector<entt::entity>& selectedEntities = {});
};


}