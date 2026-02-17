#pragma once
#include "sol/sol.hpp"
#include "entt/entt.h"

namespace rv {
// forward declaration
struct ScriptComponent;
class Scene;

class ScriptSystem
{
public:
	ScriptSystem(Scene& scene);

	void OnStart(entt::registry& registry);
	void OnUpdate(entt::registry& registry,float dt);
	void OnStop(entt::registry& registry);


	sol::state& GetState() { return m_State; }
	void BindLuaScript(ScriptComponent& sc, entt::entity& e);

private:
	sol::state m_State; // TODO: make state per scene
	Scene& m_Scene;
};

}