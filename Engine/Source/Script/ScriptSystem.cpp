#include "rvelapch.h"
#include "ScriptSystem.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "Scene/Entity.h"
#include "Physics/CollisionInfo.h"

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
    DispatchCollisionEvents();

    auto view = m_Scene.GetRegistry().view<ScriptComponent>();

    for (auto entity : view)
    {
        auto& sc = view.get<ScriptComponent>(entity);

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
        auto& sc = view.get<ScriptComponent>(entity);

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
    auto scView = m_Scene.GetRegistry().view<ScriptComponent>();
    for (auto entity : scView)
    {
        auto& sc = scView.get<ScriptComponent>(entity);

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

    //TODO: Runtime material instances
    auto mcView = m_Scene.GetRegistry().view<MaterialComponent>();
    for (auto entity : mcView)
    {
        auto& mc = mcView.get<MaterialComponent>(entity);
        mc.Reload();

    }

}

void ScriptSystem::BindLuaScript(ScriptComponent& sc, entt::entity e)
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
    sc.luaInstance["physics"] = &m_Scene.GetPhysicsSystem().GetPhysicsWorld();

    sc.OnCreate = sc.luaInstance["OnCreate"];
    sc.OnUpdate = sc.luaInstance["OnUpdate"];
    sc.OnFixedUpdate = sc.luaInstance["OnFixedUpdate"];
    sc.OnLateUpdate = sc.luaInstance["OnLateUpdate"];
    sc.OnDestroy = sc.luaInstance["OnDestroy"];

    sc.OnCollisionEnter = sc.luaInstance["OnCollisionEnter"];
    sc.OnCollisionStay = sc.luaInstance["OnCollisionStay"];
    sc.OnCollisionExit = sc.luaInstance["OnCollisionExit"];

    
}

void ScriptSystem::DispatchCollisionEvents()
{
    auto events = m_Scene.GetPhysicsSystem().FlushEvents();
    auto& reg = m_Scene.GetRegistry();

    auto dispatchEvent = [](ScriptComponent& sc, Physics::CollisionEventType type, Physics::CollisionInfo& info)
        {
            sol::function* fn = nullptr;
            const char* name = "";

            switch (type)
            {
            case Physics::CollisionEventType::ENTER: fn = &sc.OnCollisionEnter; name = "OnCollisionEnter"; break;
            case Physics::CollisionEventType::STAY:  fn = &sc.OnCollisionStay;  name = "OnCollisionStay";  break;
            case Physics::CollisionEventType::EXIT:  fn = &sc.OnCollisionExit;  name = "OnCollisionExit";  break;
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

        if (scA)
        {
            Physics::CollisionInfo info = BuildCollisionInfo(event.collision, event.entityB);
            dispatchEvent(*scA, event.eventType, info);
        }

        if (scB && reg.try_get<ScriptComponent>(event.entityB))
        {
            Physics::CollisionInfo info = BuildCollisionInfo(event.collision, event.entityA);
            info.normal *= -1.0f;
            dispatchEvent(*scB, event.eventType, info);
        }
    }
}

Physics::CollisionInfo rv::ScriptSystem::BuildCollisionInfo(const Physics::Collision& collision, entt::entity otherEntity)
{
    return { Entity(otherEntity, &m_Scene), collision.point, collision.normal };
}
