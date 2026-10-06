#include "rvelapch.h"
#include "ScriptSystem.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "Scene/Entity.h"
#include "Physics/CollisionInfo.h"
#include "SceneBindings.h"
#include <Asset/Types/ScriptAsset.h>
#include "sol/variadic_args.hpp"
#include <algorithm>
#include <cmath>

using namespace rv;



ScriptSystem::ScriptSystem(Scene& scene) : m_Scene(scene)
{
    m_ScriptEngine.Init();
}

void ScriptSystem::BindLuaScript(ScriptComponent& sc, entt::entity e)
{
    if (!sc.scriptAssetUUID.IsValid())
        return;

    auto scriptAsset = AssetManager::Get().GetAsset<ScriptAsset>(sc.scriptAssetUUID);
    if (!scriptAsset || !scriptAsset->IsValid())
    {
        LOG_ERROR("ScriptAsset not found or invalid, UUID: {}", sc.scriptAssetUUID.ToString());
        return;
    }

    sc.luaState = &m_ScriptEngine.GetState();

    sol::load_result script = sc.luaState->load(
        scriptAsset->GetSource(),
        scriptAsset->GetScriptName()
    );

    if (!script.valid())
    {
        sol::error err = script;
        LOG_ERROR("Lua load error [{}]: {}", scriptAsset->GetScriptName(), err.what());
        return;
    }

    sol::protected_function func = script;
    sol::protected_function_result result = func();
    if (!result.valid())
    {
        sol::error err = result;
        LOG_ERROR("Lua exec error [{}]: {}", scriptAsset->GetScriptName(), err.what());
        return;
    }

    sol::table scriptTable;
    if (result.valid() && result.get_type() == sol::type::table)
    {
        scriptTable = result;
    }

    sc.luaInstance = sc.luaState->create_table();
    if (scriptTable.valid())
    {
        sol::table mt = sc.luaState->create_table();
        mt["__index"] = scriptTable;
        sc.luaInstance[sol::metatable_key] = mt;

        m_SignalDefinitions.erase(e);
        sol::object signalsObject = scriptTable["signals"];
        if (signalsObject.valid() && signalsObject.get_type() == sol::type::table)
        {
            sol::table signals = signalsObject.as<sol::table>();
            for (const auto& [key, value] : signals)
            {
                if (value.get_type() == sol::type::string)
                    m_SignalDefinitions[e].insert(value.as<std::string>());
                else if (key.get_type() == sol::type::string && value.get_type() == sol::type::boolean && value.as<bool>())
                    m_SignalDefinitions[e].insert(key.as<std::string>());
            }
        }
    }

    sc.luaInstance["entity"] = Entity(e, &m_Scene);
    sc.luaInstance["scene"] = &m_Scene;
    sc.luaInstance["physics"] = &m_Scene.GetPhysicsSystem().GetPhysicsWorld();
    sc.luaInstance["EmitSignal"] = [this, e](sol::table, const std::string& signalName, sol::variadic_args va)
    {
        std::vector<sol::object> args;
        args.reserve(va.size());
        for (auto arg : va)
            args.push_back(arg.get<sol::object>());
        return EmitSignal(e, signalName, args);
    };
    sc.luaInstance["StartCoroutine"] = [this, e](sol::table, sol::function function)
    {
        return StartCoroutine(e, std::move(function));
    };
    sc.luaInstance["StopCoroutine"] = [this, e](sol::table, uint64_t coroutineId)
    {
        return StopCoroutine(e, coroutineId);
    };
    sc.luaInstance["StartTimer"] = [this, e](sol::table, float seconds, sol::protected_function callback)
    {
        return StartTimer(e, seconds, std::move(callback));
    };
    sc.luaInstance["StartRepeatingTimer"] = [this, e](sol::table, float interval, sol::protected_function callback)
    {
		if (!std::isfinite(interval) || interval <= 0.0f)
			return uint64_t{ 0 };
        return StartTimer(e, interval, std::move(callback), interval);
    };
    sc.luaInstance["CancelTimer"] = [this, e](sol::table, uint64_t timerId)
    {
        return CancelTimer(e, timerId);
    };

    // Defaults
    const auto& propDefs = ScriptEngine::GetScriptPropertyDefs(sc.scriptAssetUUID);
    for (const auto& def : propDefs)
    {
        switch (def.type)
        {
        case ScriptPropertyType::Float:       sc.luaInstance[def.name] = def.floatVal; break;
        case ScriptPropertyType::Int:         sc.luaInstance[def.name] = def.intVal; break;
        case ScriptPropertyType::Bool:        sc.luaInstance[def.name] = def.boolVal; break;
        case ScriptPropertyType::String:      sc.luaInstance[def.name] = def.stringVal; break;
        case ScriptPropertyType::Vec2:        sc.luaInstance[def.name] = def.vec2Val; break;
        case ScriptPropertyType::Vec3:        sc.luaInstance[def.name] = def.vec3Val; break;
        case ScriptPropertyType::Vec4:
        case ScriptPropertyType::Color:       sc.luaInstance[def.name] = def.vec4Val; break;
        case ScriptPropertyType::AssetHandle: sc.luaInstance[def.name] = def.assetVal; break;
        case ScriptPropertyType::Entity:
            sc.luaInstance[def.name] = def.entityVal.Resolve(m_Scene);
            break;
        }
    }

    // Overrides
    for (const auto& def : propDefs)
    {
        if (!sc.propertyValues.contains(def.name))
            continue;

        const auto& val = sc.propertyValues[def.name];
        try
        {
            switch (def.type)
            {
            case ScriptPropertyType::Float:
                if (val.is_number()) sc.luaInstance[def.name] = val.get<float>();
                break;
            case ScriptPropertyType::Int:
                if (val.is_number_integer()) sc.luaInstance[def.name] = val.get<int>();
                else if (val.is_number()) sc.luaInstance[def.name] = static_cast<int>(val.get<float>());
                break;
            case ScriptPropertyType::Bool:
                if (val.is_boolean()) sc.luaInstance[def.name] = val.get<bool>();
                break;
            case ScriptPropertyType::String:
                if (val.is_string()) sc.luaInstance[def.name] = val.get<std::string>();
                break;
            case ScriptPropertyType::Vec2:
                if (val.is_array() && val.size() >= 2)
                    sc.luaInstance[def.name] = glm::vec2(val[0].get<float>(), val[1].get<float>());
                break;
            case ScriptPropertyType::Vec3:
                if (val.is_array() && val.size() >= 3)
                    sc.luaInstance[def.name] = glm::vec3(val[0].get<float>(), val[1].get<float>(), val[2].get<float>());
                break;
            case ScriptPropertyType::Vec4:
            case ScriptPropertyType::Color:
                if (val.is_array() && val.size() >= 4)
                    sc.luaInstance[def.name] = glm::vec4(val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>());
                break;
            case ScriptPropertyType::AssetHandle:
                if (val.is_string())
                    sc.luaInstance[def.name] = AssetUUID::FromString(val.get<std::string>());
                break;
            case ScriptPropertyType::Entity:
                if (val.is_string())
                {
                    EntityHandle handle;
                    handle.uuid = std::stoull(val.get<std::string>());
                    sc.luaInstance[def.name] = handle.Resolve(m_Scene);
                }
                break;
            }
        }
        catch (const std::exception& ex)
        {
            LOG_WARN("Failed to apply script property override '{}': {}", def.name, ex.what());
        }
    }

    sc.OnCreate = sc.luaInstance["OnCreate"];
    sc.OnReady = sc.luaInstance["OnReady"];
    sc.OnUpdate = sc.luaInstance["OnUpdate"];
    sc.OnFixedUpdate = sc.luaInstance["OnFixedUpdate"];
    sc.OnLateUpdate = sc.luaInstance["OnLateUpdate"];
    sc.OnDestroy = sc.luaInstance["OnDestroy"];
    sc.OnEnabled = sc.luaInstance["OnEnabled"];
    sc.OnDisabled = sc.luaInstance["OnDisabled"];
    sc.OnScenePaused = sc.luaInstance["OnScenePaused"];
    sc.OnSceneResumed = sc.luaInstance["OnSceneResumed"];
    sc.OnCollisionEnter = sc.luaInstance["OnCollisionEnter"];
    sc.OnCollisionStay = sc.luaInstance["OnCollisionStay"];
    sc.OnCollisionExit = sc.luaInstance["OnCollisionExit"];
    sc.OnTriggerEnter = sc.luaInstance["OnTriggerEnter"];
    sc.OnTriggerStay = sc.luaInstance["OnTriggerStay"];
    sc.OnTriggerExit = sc.luaInstance["OnTriggerExit"];
    sc.OnAnimationEvent = sc.luaInstance["OnAnimationEvent"];
    sc.OnAnimationStarted = sc.luaInstance["OnAnimationStarted"];
    sc.OnAnimationFinished = sc.luaInstance["OnAnimationFinished"];
    sc.OnAnimationLooped = sc.luaInstance["OnAnimationLooped"];
    sc.OnAudioFinished = sc.luaInstance["OnAudioFinished"];
}

void ScriptSystem::InvokeLifecycleCallback(ScriptComponent& sc, sol::protected_function& callback, const char* callbackName)
{
    if (!sc.luaInstance.valid() || !callback.valid())
        return;

    sol::protected_function_result result = callback(sc.luaInstance);
    if (!result.valid())
    {
        sol::error err = result;
        LOG_ERROR("Lua {} error: {}", callbackName, err.what());
    }
}

uint64_t ScriptSystem::StartCoroutine(entt::entity owner, sol::function function)
{
	if (!m_IsRunning || !m_Scene.GetRegistry().valid(owner) || !function.valid())
		return 0;

	sol::state_view lua(function.lua_state());
	sol::function createCoroutine = lua["coroutine"]["create"];
	if (!createCoroutine.valid())
		return 0;

	sol::thread thread = createCoroutine(function);
	if (!thread.valid())
		return 0;

	const uint64_t id = m_NextScheduledTaskId++;
	m_Coroutines.emplace(id, CoroutineTask{ id, owner, sol::coroutine(thread), 0.0f, true });
	return id;
}

bool ScriptSystem::StopCoroutine(entt::entity owner, uint64_t coroutineId)
{
	auto it = m_Coroutines.find(coroutineId);
	if (it == m_Coroutines.end() || it->second.owner != owner)
		return false;

	m_Coroutines.erase(it);
	return true;
}

uint64_t ScriptSystem::StartTimer(entt::entity owner, float seconds, sol::protected_function callback, float interval)
{
	if (!m_IsRunning || !m_Scene.GetRegistry().valid(owner) || !callback.valid())
		return 0;

	if (!std::isfinite(seconds)) seconds = 0.0f;
	if (!std::isfinite(interval)) interval = 0.0f;
	seconds = std::max(seconds, 0.0f);
	interval = std::max(interval, 0.0f);
	if (interval > 0.0f)
		seconds = std::max(seconds, interval);

	const uint64_t id = m_NextScheduledTaskId++;
	m_Timers.emplace(id, TimerTask{ id, owner, std::move(callback), seconds, interval });
	return id;
}

bool ScriptSystem::CancelTimer(entt::entity owner, uint64_t timerId)
{
	auto it = m_Timers.find(timerId);
	if (it == m_Timers.end() || it->second.owner != owner)
		return false;

	m_Timers.erase(it);
	return true;
}

void ScriptSystem::AdvanceScheduledTasks(float dt)
{
	std::vector<uint64_t> timerIds;
	timerIds.reserve(m_Timers.size());
	for (const auto& [id, timer] : m_Timers)
		timerIds.push_back(id);

	for (uint64_t id : timerIds)
	{
		auto it = m_Timers.find(id);
		if (it == m_Timers.end())
			continue;

		TimerTask& timer = it->second;
		if (!m_Scene.GetRegistry().valid(timer.owner) || !m_Scene.IsEntityActive(timer.owner))
			continue;

		timer.waitRemaining -= dt;
		if (timer.waitRemaining > 0.0f)
			continue;

		const entt::entity owner = timer.owner;
		sol::protected_function callback = timer.callback;
		if (timer.interval > 0.0f)
			timer.waitRemaining = timer.interval;
		else
			m_Timers.erase(it);

		auto* sc = m_Scene.GetRegistry().try_get<ScriptComponent>(owner);
		if (!sc || !sc->luaInstance.valid())
			continue;

		sol::protected_function_result result = callback(sc->luaInstance);
		if (!result.valid())
		{
			sol::error err = result;
			LOG_ERROR("Lua timer callback error: {}", err.what());
			m_Timers.erase(id);
		}
	}

	std::vector<uint64_t> coroutineIds;
	coroutineIds.reserve(m_Coroutines.size());
	for (const auto& [id, task] : m_Coroutines)
		coroutineIds.push_back(id);

	for (uint64_t id : coroutineIds)
	{
		auto it = m_Coroutines.find(id);
		if (it == m_Coroutines.end())
			continue;

		CoroutineTask& task = it->second;
		if (!m_Scene.GetRegistry().valid(task.owner) || !m_Scene.IsEntityActive(task.owner))
			continue;

		task.waitRemaining = std::max(0.0f, task.waitRemaining - dt);
		if (task.waitRemaining > 0.0f)
			continue;

		auto* sc = m_Scene.GetRegistry().try_get<ScriptComponent>(task.owner);
		if (!sc || !sc->luaInstance.valid())
		{
			m_Coroutines.erase(it);
			continue;
		}

		const entt::entity owner = task.owner;
		const bool firstResume = task.firstResume;
		sol::coroutine coroutine = task.coroutine;
		sol::table instance = sc->luaInstance;
		task.firstResume = false;

		sol::protected_function_result result;
		if (firstResume)
			result = coroutine(instance);
		else
			result = coroutine();

		it = m_Coroutines.find(id);
		if (it == m_Coroutines.end())
			continue;

		if (!result.valid())
		{
			sol::error err = result;
			LOG_ERROR("Lua coroutine error on entity {}: {}", static_cast<uint32_t>(owner), err.what());
			m_Coroutines.erase(it);
			continue;
		}

		if (result.status() == sol::call_status::yielded)
		{
			float waitSeconds = 0.0f;
			if (result.return_count() > 0 && result.get_type() == sol::type::number)
				waitSeconds = result.get<float>();
			if (!std::isfinite(waitSeconds)) waitSeconds = 0.0f;
			it->second.waitRemaining = std::max(waitSeconds, 0.0f);
		}
		else
		{
			m_Coroutines.erase(it);
		}
	}
}

void ScriptSystem::ClearScheduledTasksForEntity(entt::entity entity)
{
	std::erase_if(m_Coroutines, [entity](const auto& entry) { return entry.second.owner == entity; });
	std::erase_if(m_Timers, [entity](const auto& entry) { return entry.second.owner == entity; });
}

void ScriptSystem::EnsureScriptInitialized(ScriptComponent& sc, entt::entity entity)
{
    if (sc.runtimeInitialized || sc.runtimeDestroying)
        return;

    BindLuaScript(sc, entity);
    if (!sc.luaInstance.valid())
        return;

    sc.runtimeInitialized = true;
    InvokeLifecycleCallback(sc, sc.OnCreate, "OnCreate");
}

void ScriptSystem::OnEntityActivationChanged(entt::entity entity, bool active)
{
    if (!m_IsRunning)
		return;

    auto* sc = m_Scene.GetRegistry().try_get<ScriptComponent>(entity);
    if (!sc || sc->runtimeDestroying)
        return;

    if (active)
    {
        EnsureScriptInitialized(*sc, entity);
        if (m_Scene.IsEntityActive(entity) && sc->runtimeInitialized && !sc->runtimeEnabled)
        {
            sc->runtimeEnabled = true;
            InvokeLifecycleCallback(*sc, sc->OnEnabled, "OnEnabled");
        }
        if (m_IsReady && m_Scene.IsEntityActive(entity) && sc->runtimeInitialized && !sc->runtimeReady)
        {
            sc->runtimeReady = true;
            InvokeLifecycleCallback(*sc, sc->OnReady, "OnReady");
        }
        if (m_Scene.GetState() == SceneState::PAUSE && sc->runtimeInitialized && !sc->runtimeScenePaused)
        {
            sc->runtimeScenePaused = true;
            InvokeLifecycleCallback(*sc, sc->OnScenePaused, "OnScenePaused");
        }
    }
    else if (sc->runtimeEnabled)
    {
        sc->runtimeEnabled = false;
        InvokeLifecycleCallback(*sc, sc->OnDisabled, "OnDisabled");
    }
}

void ScriptSystem::OnEntityDestroyed(entt::entity entity)
{
    auto* sc = m_Scene.GetRegistry().try_get<ScriptComponent>(entity);
    if (!sc)
    {
        DisconnectSignalsForEntity(entity);
		ClearScheduledTasksForEntity(entity);
        return;
    }
    if (!sc->runtimeInitialized || sc->runtimeDestroying)
    {
        if (!sc->runtimeDestroying)
            DisconnectSignalsForEntity(entity);
		ClearScheduledTasksForEntity(entity);
        return;
    }

    sc->runtimeDestroying = true;

    if (sc->runtimeEnabled)
    {
        sc->runtimeEnabled = false;
        InvokeLifecycleCallback(*sc, sc->OnDisabled, "OnDisabled");
    }
    InvokeLifecycleCallback(*sc, sc->OnDestroy, "OnDestroy");
    sc->runtimeInitialized = false;
    sc->runtimeReady = false;
    sc->runtimeScenePaused = false;
    sc->runtimeDestroying = false;
    DisconnectSignalsForEntity(entity);
	ClearScheduledTasksForEntity(entity);
}

uint64_t ScriptSystem::ConnectSignal(entt::entity source, const std::string& signalName, entt::entity target, const std::string& methodName)
{
	if (!m_IsRunning || signalName.empty() || methodName.empty() ||
		!m_Scene.GetRegistry().valid(source) || !m_Scene.GetRegistry().valid(target) ||
		!m_Scene.GetRegistry().any_of<ScriptComponent>(source) || !m_Scene.GetRegistry().any_of<ScriptComponent>(target))
		return 0;

	for (const auto& connection : m_SignalConnections)
	{
		if (connection.source == source && connection.signalName == signalName &&
			connection.target == target && connection.methodName == methodName)
			return connection.id;
	}

	const uint64_t id = m_NextSignalConnectionId++;
	m_SignalConnections.push_back({ id, source, signalName, target, methodName });
	return id;
}

bool ScriptSystem::DisconnectSignal(entt::entity source, uint64_t connectionId)
{
	auto it = std::find_if(m_SignalConnections.begin(), m_SignalConnections.end(),
		[source, connectionId](const SignalConnection& connection)
		{
			return connection.id == connectionId && connection.source == source;
		});
	if (it == m_SignalConnections.end())
		return false;

	m_SignalConnections.erase(it);
	return true;
}

bool ScriptSystem::EmitSignal(entt::entity source, const std::string& signalName, const std::vector<sol::object>& args)
{
	if (!m_IsRunning || !m_Scene.GetRegistry().valid(source))
		return false;

	auto definitions = m_SignalDefinitions.find(source);
	if (definitions == m_SignalDefinitions.end() || !definitions->second.contains(signalName))
	{
		LOG_WARN("Cannot emit undefined Lua signal '{}' from entity {}", signalName, static_cast<uint32_t>(source));
		return false;
	}

	std::vector<SignalConnection> listeners;
	for (const auto& connection : m_SignalConnections)
	{
		if (connection.source == source && connection.signalName == signalName)
			listeners.push_back(connection);
	}

	for (const auto& listener : listeners)
	{
		const bool stillConnected = std::any_of(m_SignalConnections.begin(), m_SignalConnections.end(),
			[&listener](const SignalConnection& connection) { return connection.id == listener.id; });
		if (!stillConnected || !m_Scene.GetRegistry().valid(listener.target))
			continue;

		auto* targetScript = m_Scene.GetRegistry().try_get<ScriptComponent>(listener.target);
		if (!targetScript || !targetScript->luaInstance.valid())
			continue;

		sol::object methodObject = targetScript->luaInstance[listener.methodName];
		if (!methodObject.valid() || methodObject.get_type() != sol::type::function)
		{
			LOG_WARN("Lua signal receiver method '{}' was not found on entity {}", listener.methodName, static_cast<uint32_t>(listener.target));
			continue;
		}

		sol::protected_function method = methodObject;
		sol::protected_function_result result = method(targetScript->luaInstance, sol::as_args(args));
		if (!result.valid())
		{
			sol::error err = result;
			LOG_ERROR("Lua signal '{}' callback '{}' error: {}", signalName, listener.methodName, err.what());
		}
	}

	return true;
}

void ScriptSystem::DisconnectSignalsForEntity(entt::entity entity)
{
	m_SignalDefinitions.erase(entity);
	std::erase_if(m_SignalConnections, [entity](const SignalConnection& connection)
		{
			return connection.source == entity || connection.target == entity;
		});
}

void ScriptSystem::ClearSignals()
{
	m_SignalDefinitions.clear();
	m_SignalConnections.clear();
}

void ScriptSystem::OnScenePaused()
{
    auto view = m_Scene.GetRegistry().view<ScriptComponent>();
    for (auto entity : view)
    {
        auto& sc = view.get<ScriptComponent>(entity);
        if (!sc.runtimeInitialized || sc.runtimeScenePaused)
            continue;

        sc.runtimeScenePaused = true;
        InvokeLifecycleCallback(sc, sc.OnScenePaused, "OnScenePaused");
    }
}

void ScriptSystem::OnSceneResumed()
{
    auto view = m_Scene.GetRegistry().view<ScriptComponent>();
    for (auto entity : view)
    {
        auto& sc = view.get<ScriptComponent>(entity);
        if (!sc.runtimeInitialized || !sc.runtimeScenePaused)
            continue;

        sc.runtimeScenePaused = false;
        InvokeLifecycleCallback(sc, sc.OnSceneResumed, "OnSceneResumed");
    }
}

void ScriptSystem::OnStart()
{
	m_IsRunning = true;
	m_IsReady = false;
    auto view = m_Scene.GetRegistry().view<ScriptComponent>();
    for (auto entity : view)
    {
        if (!m_Scene.IsEntityActive(entity))
            continue;

        auto& sc = view.get<ScriptComponent>(entity);
        EnsureScriptInitialized(sc, entity);
        if (m_Scene.IsEntityActive(entity) && sc.runtimeInitialized && !sc.runtimeEnabled)
        {
            sc.runtimeEnabled = true;
            InvokeLifecycleCallback(sc, sc.OnEnabled, "OnEnabled");
        }
    }

    m_IsReady = true;
    auto readyView = m_Scene.GetRegistry().view<ScriptComponent>();
    for (auto entity : readyView)
    {
        auto& sc = readyView.get<ScriptComponent>(entity);
        if (!m_Scene.IsEntityActive(entity) || !sc.runtimeInitialized || sc.runtimeReady)
            continue;

        sc.runtimeReady = true;
        InvokeLifecycleCallback(sc, sc.OnReady, "OnReady");
    }
}

void ScriptSystem::OnUpdate(float dt)
{
    DispatchCollisionEvents();
    DispatchAnimationEvents();
    DispatchAudioEvents();
	AdvanceScheduledTasks(dt);

    auto view = m_Scene.GetRegistry().view<ScriptComponent>();

    for (auto entity : view)
    {
        if (!m_Scene.IsEntityActive(entity))
            continue;

        auto& sc = view.get<ScriptComponent>(entity);

        if (!sc.luaInstance.valid())
        {
            OnEntityActivationChanged(entity, true);
        }

        if (!m_Scene.IsEntityActive(entity))
            continue;

        if (sc.luaInstance.valid())
        {
            if (sc.OnUpdate.valid())
            {
                sol::protected_function_result result = sc.OnUpdate(sc.luaInstance, dt);
                if (!result.valid())
                {
                    sol::error err = result;
                    LOG_ERROR("Lua OnUpdate error: {}", err.what());
                }
            }
        }
    }
}
void ScriptSystem::OnFixedUpdate(float dt)
{
    auto view = m_Scene.GetRegistry().view<ScriptComponent>();

    for (auto entity : view)
    {
        if (!m_Scene.IsEntityActive(entity))
            continue;

        auto& sc = view.get<ScriptComponent>(entity);

        if (!sc.luaInstance.valid())
        {
            OnEntityActivationChanged(entity, true);
        }

        if (!m_Scene.IsEntityActive(entity))
            continue;

        if (sc.luaInstance.valid())
        {
            if (sc.OnFixedUpdate.valid())
            {
                sol::protected_function_result result = sc.OnFixedUpdate(sc.luaInstance, dt);
                if (!result.valid())
                {
                    sol::error err = result;
                    LOG_ERROR("Lua OnFixedUpdate error: {}", err.what());
                }
            }
        }
    }

}
void ScriptSystem::OnLateUpdate(float dt)
{
    auto view = m_Scene.GetRegistry().view<ScriptComponent>();

    for (auto entity : view)
    {
        if (!m_Scene.IsEntityActive(entity))
            continue;

        auto& sc = view.get<ScriptComponent>(entity);

        if (sc.luaInstance.valid())
        {
            if (sc.OnLateUpdate.valid())
            {
                sol::protected_function_result result = sc.OnLateUpdate(sc.luaInstance, dt);
                if (!result.valid())
                {
                    sol::error err = result;
                    LOG_ERROR("Lua OnLateUpdate error: {}", err.what());
                }
            }
        }
    }

}

void ScriptSystem::OnStop()
{
    m_IsRunning = false;
	m_IsReady = false;
    auto scView = m_Scene.GetRegistry().view<ScriptComponent>();
    for (auto entity : scView)
    {
        auto& sc = scView.get<ScriptComponent>(entity);

        OnEntityDestroyed(entity);
    }
    auto mcView = m_Scene.GetRegistry().view<MaterialComponent>();
    for (auto entity : mcView)
    {
        auto& mc = mcView.get<MaterialComponent>(entity);
        mc.Reload();
        //TODO: WTF IS THIS?
    }
	ClearSignals();
	m_Coroutines.clear();
	m_Timers.clear();

}



void ScriptSystem::DispatchCollisionEvents()
{
    auto events = m_Scene.GetPhysicsSystem().FlushEvents();
    auto& reg = m_Scene.GetRegistry();

    auto dispatchEvent = [](ScriptComponent& sc, Physics::CollisionEventType type, Physics::CollisionInfo& info)
        {
            sol::protected_function* fn = nullptr;
            const char* name = "";

            if (info.isTrigger)
            {
                switch (type)
                {
                case Physics::CollisionEventType::ENTER: fn = &sc.OnTriggerEnter; name = "OnTriggerEnter"; break;
                case Physics::CollisionEventType::STAY:  fn = &sc.OnTriggerStay;  name = "OnTriggerStay";  break;
                case Physics::CollisionEventType::EXIT:  fn = &sc.OnTriggerExit; name = "OnTriggerExit"; break;
                }
            }
            else
            {
                switch (type)
                {
                case Physics::CollisionEventType::ENTER: fn = &sc.OnCollisionEnter; name = "OnCollisionEnter"; break;
                case Physics::CollisionEventType::STAY:  fn = &sc.OnCollisionStay;  name = "OnCollisionStay";  break;
                case Physics::CollisionEventType::EXIT:  fn = &sc.OnCollisionExit;  name = "OnCollisionExit";  break;
                }
            }

            if (fn && fn->valid())
            {
                sol::protected_function_result result = (*fn)(sc.luaInstance, info);
                if (!result.valid())
                {
                    sol::error err = result;
                    LOG_ERROR("Lua {} error: {}", name, err.what());
                }
            }
        };

    for (auto& event : events)
    {
        ScriptComponent* scA = reg.try_get<ScriptComponent>(event.entityA);
        ScriptComponent* scB = reg.try_get<ScriptComponent>(event.entityB);

        if (scA && scA->luaInstance.valid() && m_Scene.IsEntityActive(event.entityA))
        {
            Physics::CollisionInfo info = BuildCollisionInfo(event.collision, event.entityB, event.isTrigger);
            dispatchEvent(*scA, event.eventType, info);
        }

        if (scB && scB->luaInstance.valid() && m_Scene.IsEntityActive(event.entityB))
        {
            Physics::CollisionInfo info = BuildCollisionInfo(event.collision, event.entityA, event.isTrigger);
            info.collision.normal *= -1.0f;
            dispatchEvent(*scB, event.eventType, info);
        }
    }
}

Physics::CollisionInfo ScriptSystem::BuildCollisionInfo(const Physics::Collision& collision, entt::entity otherEntity, bool isTrigger)
{
    return { Entity(otherEntity, &m_Scene), collision, isTrigger };
}

void ScriptSystem::DispatchAnimationEvents()
{
    auto events = m_Scene.GetAnimationSystem().FlushEvents();
    auto& reg = m_Scene.GetRegistry();
    auto scriptView = reg.view<ScriptComponent>();

    for (const auto& ev : events)
    {
        if (!reg.valid(ev.entity))
            continue;

        Entity source(ev.entity, &m_Scene);

        for (auto entity : scriptView)
        {
            auto& sc = scriptView.get<ScriptComponent>(entity);
            if (!sc.luaInstance.valid() || !m_Scene.IsEntityActive(entity))
                continue;

            sol::protected_function_result result;

            switch (ev.type)
            {
            case Animation::EventType::Started:
                if (sc.OnAnimationStarted.valid())
                    result = sc.OnAnimationStarted(sc.luaInstance, ev.clipName);
                break;

            case Animation::EventType::Looped:
                if (sc.OnAnimationLooped.valid())
                    result = sc.OnAnimationLooped(sc.luaInstance, ev.clipName);
                break;

            case Animation::EventType::Finished:
                if (sc.OnAnimationFinished.valid())
                    result = sc.OnAnimationFinished(sc.luaInstance, ev.clipName);
                break;

            case Animation::EventType::Triggered:
                if (sc.OnAnimationEvent.valid())
                    result = sc.OnAnimationEvent(sc.luaInstance, source, ev.eventName, ev.parameter);
                break;
            }

            if (result.valid() == false && result.status() != sol::call_status::ok)
            {
                sol::error err = result;
                LOG_ERROR("Lua Animation Event error: {}", err.what());
            }
        }
    }
}

void ScriptSystem::DispatchAudioEvents()
{
    auto events = m_Scene.GetAudioSystem().FlushEvents();
    auto& reg = m_Scene.GetRegistry();

    for (const auto& ev : events)
    {
        if (!reg.valid(ev.entity))
            continue;

        ScriptComponent* sc = reg.try_get<ScriptComponent>(ev.entity);
        if (!sc || !sc->luaInstance.valid() || !m_Scene.IsEntityActive(ev.entity))
            continue;

        if (ev.type == Audio::EventType::FINISHED && sc->OnAudioFinished.valid())
        {
            sol::protected_function_result result = sc->OnAudioFinished(sc->luaInstance);
            if (!result.valid())
            {
                sol::error err = result;
                LOG_ERROR("Lua OnAudioFinished error: {}", err.what());
            }
        }
    }
}

