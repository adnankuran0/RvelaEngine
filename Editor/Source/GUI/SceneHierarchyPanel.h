#pragma once
#include "entt/entt.h"

class Engine;

class SceneHierarchyPanel
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity);
};
