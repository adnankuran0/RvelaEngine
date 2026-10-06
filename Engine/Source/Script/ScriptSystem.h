#pragma once
#include "sol/sol.hpp"
#include "entt/entt.h"
#include "ScriptEngine.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rv::Physics {
struct CollisionInfo;
struct Collision;
}

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
	void OnFixedUpdate(float dt);
	void OnLateUpdate(float dt);
	void OnStop();
	void OnScenePaused();
	void OnSceneResumed();
	void OnEntityActivationChanged(entt::entity entity, bool active);
	void OnEntityDestroyed(entt::entity entity);
	uint64_t ConnectSignal(entt::entity source, const std::string& signalName, entt::entity target, const std::string& methodName);
	bool DisconnectSignal(entt::entity source, uint64_t connectionId);
	bool EmitSignal(entt::entity source, const std::string& signalName, const std::vector<sol::object>& args);

	void BindLuaScript(ScriptComponent& sc, entt::entity e);

private:
	void DispatchCollisionEvents();
	void DispatchAnimationEvents();
	void DispatchAudioEvents();
	void EnsureScriptInitialized(ScriptComponent& sc, entt::entity entity);
	void InvokeLifecycleCallback(ScriptComponent& sc, sol::protected_function& callback, const char* callbackName);
	void DisconnectSignalsForEntity(entt::entity entity);
	void ClearSignals();
	Physics::CollisionInfo BuildCollisionInfo(const Physics::Collision& collision, entt::entity otherEntity, bool isTrigger);

	struct SignalConnection
	{
		uint64_t id;
		entt::entity source;
		std::string signalName;
		entt::entity target;
		std::string methodName;
	};
	
	ScriptEngine m_ScriptEngine;
	Scene& m_Scene;
	bool m_IsRunning = false;
	bool m_IsReady = false;
	uint64_t m_NextSignalConnectionId = 1;
	std::unordered_map<entt::entity, std::unordered_set<std::string>> m_SignalDefinitions;
	std::vector<SignalConnection> m_SignalConnections;
};

}
