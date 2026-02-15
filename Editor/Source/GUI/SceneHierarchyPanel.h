#pragma once
#include "entt/entt.h"

namespace rv { 

class Engine;

class SceneHierarchyPanel
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity);
};

}