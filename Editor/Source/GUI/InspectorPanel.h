#pragma once
#include "entt/entt.h"

namespace rv {

class Engine;

class InspectorPanel
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity);
};


}