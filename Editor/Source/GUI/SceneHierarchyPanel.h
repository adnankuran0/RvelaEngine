#pragma once
#include "entt/entt.h"
#include "Asset/AssetUUID.h"
#include <unordered_map>


namespace rv { 

class Engine;
class AssetRegistry;

class SceneHierarchyPanel
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity);
};

}