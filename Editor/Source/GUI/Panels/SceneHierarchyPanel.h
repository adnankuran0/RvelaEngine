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
	void Draw(Engine* engine);

private:
	entt::entity m_RenamingEntity = entt::null;
	char m_RenameBuffer[256] = {};
	bool m_FocusRenameInput = false;
};

}
