#pragma once
#include "sol/sol.hpp"
#include "entt/entt.h"

// forward declaration
struct ScriptComponent; 
class Scene;

class ScriptEngine
{
public:
	ScriptEngine();
	sol::state& GetState() { return m_State; }
	void BindLuaScript(ScriptComponent& sc, entt::entity& e, Scene& scene);

private:
	sol::state m_State; // TODO: make state per scene

};