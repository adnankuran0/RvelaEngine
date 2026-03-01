#include "rvelapch.h"
#include "ScriptSystem.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Entity.h"
#include "Input/Input.h"

using namespace rv;

ScriptSystem::ScriptSystem(Scene& scene) : m_Scene(scene)
{
    m_State.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::os);

    m_State.new_usertype<glm::vec3>("vec3",
        sol::constructors<
        glm::vec3(),
        glm::vec3(float, float, float)
        >(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z
    );

    m_State.new_usertype<TransformComponent>("TransformComponent",
        "GetPosition", &TransformComponent::GetPosition,
        "SetPosition", &TransformComponent::SetPosition,
        "Translate", &TransformComponent::Translate,
        "GetRotation", &TransformComponent::GetRotation,
        "SetRotation", &TransformComponent::SetRotation,
        "GetEulerRotation", &TransformComponent::GetEulerRotation,
        "SetEulerRotation", &TransformComponent::SetEulerRotation,
        "GetScale", &TransformComponent::GetScale,
        "SetScale", &TransformComponent::SetScale
    );

    m_State.new_usertype<Entity>("Entity",
        "GetComponent", [](Entity& e) -> TransformComponent& {
            return e.GetComponent<TransformComponent>();
        },
        "HasComponent", [](Entity& e) -> bool {
            return e.HasComponent<TransformComponent>();
        },
        "GetName", &Entity::GetName
    );

    m_State["KeyCode"] = m_State.create_table();
    m_State["MouseCode"] = m_State.create_table();

    #define BIND_KEY(k) m_State["KeyCode"][#k] = static_cast<int>(KeyCode::k);
    #define BIND_MOUSE(m) m_State["MouseCode"][#m] = static_cast<int>(MouseCode::m);

    BIND_KEY(W)
    BIND_KEY(A)
    BIND_KEY(S)
    BIND_KEY(D)
    BIND_KEY(Space)
    BIND_KEY(LeftShift)

    BIND_MOUSE(ButtonLeft)
    BIND_MOUSE(ButtonRight)

    #undef BIND_KEY
    #undef BIND_MOUSE

    m_State["Input"] = m_State.create_table();

    m_State["Input"]["IsKeyPressed"] = [](int key)
        {
            return Input::IsKeyPressed(static_cast<KeyCode>(key));
        };

    m_State["Input"]["IsKeyJustPressed"] = [](int key)
        {
            return Input::IsKeyJustPressed(static_cast<KeyCode>(key));
        };

    m_State["Input"]["IsMouseButtonPressed"] = [](int button)
        {
            return Input::IsMouseButtonPressed(static_cast<MouseCode>(button));
        };

    m_State["Input"]["GetMousePosition"] = []()
        {
            glm::vec2 pos = Input::GetMousePosition();
            return glm::vec3(pos.x, pos.y, 0.0f);
        };
}

void ScriptSystem::OnStart(entt::registry& registry)
{
    auto view = registry.view<ScriptComponent>();
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

void ScriptSystem::OnUpdate(entt::registry& registry, float dt)
{
    auto view = registry.view<ScriptComponent>();

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

void ScriptSystem::OnStop(entt::registry& registry)
{
    auto view = registry.view<ScriptComponent>();
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
    sc.luaState = &m_State;

    sol::load_result script = sc.luaState->load_file(sc.luaFile);
    if (!script.valid()) { LOG_ERROR(script); return; }

    sol::protected_function func = script;
    sol::protected_function_result result = func();
    if (!result.valid()) { LOG_ERROR(result); return; }

    sc.luaInstance = result;

    sc.luaInstance["entity"] = Entity(e, &m_Scene);

    sc.OnCreate = sc.luaInstance["OnCreate"];
    sc.OnUpdate = sc.luaInstance["OnUpdate"];
    sc.OnDestroy = sc.luaInstance["OnDestroy"];

    if (sc.OnCreate.valid())
        sc.OnCreate(sc.luaInstance);
}
