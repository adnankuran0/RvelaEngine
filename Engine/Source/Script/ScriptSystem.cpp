#include "rvelapch.h"
#include "ScriptSystem.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Entity.h"

using namespace rv;

ScriptSystem::ScriptSystem(Scene& scene) : m_Scene(scene)
{
    m_ScriptEngine.Init();
}

void ScriptSystem::OnStart()
{
    auto view = m_Scene.GetRegistry().view<ScriptComponent>();
    for (auto entity : view)
    {
        auto& sc = view.get<ScriptComponent>(entity);
        BindLuaScript(sc, entity);
        if (sc.OnCreate.valid())
        {
            sol::protected_function_result result = sc.OnCreate(sc.luaInstance);
            if (!result.valid())
            {
                sol::error err = result;
                LOG_ERROR("Lua OnCreate error: {}", err.what());
            }
        }
    }
}

void ScriptSystem::OnUpdate(float dt)
{
    auto view = m_Scene.GetRegistry().view<ScriptComponent>();

    for (auto entity : view)
    {
        auto& sc = view.get<ScriptComponent>(entity);

        if (sc.luaInstance.valid())
        {
            sol::protected_function onUpdate = sc.luaInstance["OnUpdate"];
            if (onUpdate.valid())
            {
                sol::protected_function_result result = onUpdate(sc.luaInstance, dt);
                if (!result.valid())
                {
                    sol::error err = result;
                    LOG_ERROR("Lua OnUpdate error: {}", err.what());
                }
            }
        }
    }

}

void ScriptSystem::OnStop()
{
    auto view = m_Scene.GetRegistry().view<ScriptComponent>();
    for (auto entity : view)
    {
        auto& sc = view.get<ScriptComponent>(entity);

        if (sc.OnDestroy.valid())
        {
            sol::protected_function_result result = sc.OnDestroy(sc.luaInstance);
            if (!result.valid())
            {
                sol::error err = result;
                LOG_ERROR("Lua OnDestroy error: {}", err.what());
            }
        }
    }
}

void ScriptSystem::BindLuaScript(ScriptComponent& sc, entt::entity& e)
{
    sc.luaState = &m_ScriptEngine.GetState();

    sol::load_result script = sc.luaState->load_file(sc.luaFile);
    if (!script.valid()) { LOG_ERROR(script); return; }

    sol::protected_function func = script;
    sol::protected_function_result result = func();
    if (!result.valid()) { LOG_ERROR(result); return; }

    sc.luaInstance = result;

    sc.luaInstance["entity"] = Entity(e, &m_Scene);
    sc.luaInstance["scene"] = &m_Scene;

    sc.OnCreate = sc.luaInstance["OnCreate"];
    sc.OnUpdate = sc.luaInstance["OnUpdate"];
    sc.OnDestroy = sc.luaInstance["OnDestroy"];

    
}
