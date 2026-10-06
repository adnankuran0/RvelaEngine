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
	uint64_t StartCoroutine(entt::entity owner, sol::function function);
	bool StopCoroutine(entt::entity owner, uint64_t coroutineId);
	uint64_t StartTimer(entt::entity owner, float seconds, sol::protected_function callback, float interval = 0.0f);
	bool CancelTimer(entt::entity owner, uint64_t timerId);
	void AdvanceScheduledTasks(float dt);
	void ClearScheduledTasksForEntity(entt::entity entity);
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

	struct CoroutineTask
	{
		uint64_t id = 0;
		entt::entity owner = entt::null;
		sol::coroutine coroutine;
		float waitRemaining = 0.0f;
		bool firstResume = true;
	};

	struct TimerTask
	{
		uint64_t id = 0;
		entt::entity owner = entt::null;
		sol::protected_function callback;
		float waitRemaining = 0.0f;
		float interval = 0.0f;
	};
	
	ScriptEngine m_ScriptEngine;
	Scene& m_Scene;
	bool m_IsRunning = false;
	bool m_IsReady = false;
	uint64_t m_NextSignalConnectionId = 1;
	uint64_t m_NextScheduledTaskId = 1;
	std::unordered_map<entt::entity, std::unordered_set<std::string>> m_SignalDefinitions;
	std::vector<SignalConnection> m_SignalConnections;
	std::unordered_map<uint64_t, CoroutineTask> m_Coroutines;
	std::unordered_map<uint64_t, TimerTask> m_Timers;
};

}
