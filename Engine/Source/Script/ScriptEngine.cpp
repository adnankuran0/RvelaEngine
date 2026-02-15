#include "rvelapch.h"
#include "ScriptEngine.h"
#include "Scene/Components/ScriptComponent.h"
#include "Scene/Entity.h"

namespace rv {

ScriptEngine::ScriptEngine()
{
    m_State.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::os);

    m_State.new_usertype<glm::vec3>("vec3",
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
}


void ScriptEngine::BindLuaScript(ScriptComponent& sc, entt::entity& e,Scene& scene)
{
    sc.luaState = &m_State;

    sol::load_result script = sc.luaState->load_file(sc.luaFile);
    if (!script.valid()) { LOG_ERROR(script); return; }

    sol::protected_function func = script;
    sol::protected_function_result result = func();
    if (!result.valid()) { LOG_ERROR(result); return; }

    sc.luaInstance = result;

    sc.luaInstance["entity"] = Entity(e, &scene);

    sc.OnCreate = sc.luaInstance["OnCreate"];
    sc.OnUpdate = sc.luaInstance["OnUpdate"];
    sc.OnDestroy = sc.luaInstance["OnDestroy"];

    if (sc.OnCreate.valid())
        sc.OnCreate(sc.luaInstance);
}

}