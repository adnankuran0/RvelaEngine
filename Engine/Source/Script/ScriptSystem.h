#pragma once
#include "sol/sol.hpp"
#include "entt/entt.h"
#include "ScriptEngine.h"

namespace rv {
// forward declaration
struct ScriptComponent;
class Scene;

class ScriptSystem
{
public:
	ScriptSystem(Scene& scene);

	void OnStart();
	void OnUpdate(float dt);
	void OnStop();

	void BindLuaScript(ScriptComponent& sc, entt::entity& e);

private:
	ScriptEngine m_ScriptEngine;
	Scene& m_Scene;
};

}