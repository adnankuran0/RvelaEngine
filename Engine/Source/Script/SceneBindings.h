#pragma once
#include "sol/forward.hpp"
#include "Scene/Entity.h"

namespace rv {
	class Scene;
}

namespace rv::LuaBindings {

	Entity InstantiatePrefabHelper(Scene& scene, sol::object prefabObj,
		sol::optional<sol::object> posObj,
		sol::optional<sol::object> rotObj,
		sol::optional<sol::object> parentObj);

	void RegisterSceneAPI(sol::state& lua);

}
