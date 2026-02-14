#pragma once
#include "entt/entt.h"

class Engine;

class InspectorPanel
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity);
};
