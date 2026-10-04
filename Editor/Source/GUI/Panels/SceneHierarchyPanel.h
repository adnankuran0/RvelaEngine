#pragma once
#include "entt/entt.h"
#include "Asset/AssetUUID.h"
#include <unordered_map>


#include <vector>

namespace rv { 

class Engine;
class AssetRegistry;

class SceneHierarchyPanel
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity, std::vector<entt::entity>& selectedEntities);
};

}