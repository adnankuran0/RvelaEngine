#include "rvelapch.h"
#include "ScriptEngine.h"
#include "Scene/Entity.h"
#include "Input/Input.h"
#include "ScriptBindings.h"

using namespace rv;

void ScriptEngine::Init()
{
    m_State.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::os);

    LuaBindings::RegisterMath(m_State);
    LuaBindings::RegisterCoreTypes(m_State);


    m_State["KeyCode"] = m_State.create_table();
    m_State["MouseCode"] = m_State.create_table();

#define BIND_KEY(k) m_State["KeyCode"][#k] = static_cast<int>(KeyCode::k);
#define BIND_MOUSE(m) m_State["MouseCode"][#m] = static_cast<int>(MouseCode::m);

    BIND_KEY(W)
        BIND_KEY(A)
        BIND_KEY(S)
        BIND_KEY(D)
        BIND_KEY(Space)
        BIND_KEY(Escape)
        BIND_KEY(LeftShift)

        BIND_MOUSE(ButtonLeft)
        BIND_MOUSE(ButtonRight)

#undef BIND_KEY
#undef BIND_MOUSE

        m_State["MouseMode"] = m_State.create_table();

    m_State["MouseMode"]["VISIBLE"] = static_cast<int>(Input::MouseMode::VISIBLE);
    m_State["MouseMode"]["HIDDEN"] = static_cast<int>(Input::MouseMode::HIDDEN);
    m_State["MouseMode"]["CAPTURED"] = static_cast<int>(Input::MouseMode::CAPTURED);


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

    m_State["Input"]["SetMouseMode"] = [](int mode)
        {
            Input::SetMouseMode(static_cast<Input::MouseMode>(mode));
        };

    m_State["Scene"] = m_State.create_table();

    m_State.new_usertype<Scene>("Scene",
        "FindEntityByName", &Scene::GetEntityByName
    );
}