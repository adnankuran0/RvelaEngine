#include "rvelapch.h"
#include "ScriptEngine.h"
#include "Scene/Entity.h"
#include "ScriptBindings.h"

using namespace rv;

void ScriptEngine::Init()
{
    m_State.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::os);

    LuaBindings::RegisterMath(m_State);
    LuaBindings::RegisterCoreTypes(m_State);
    LuaBindings::RegisterComponents(m_State);
    LuaBindings::RegisterInputAPI(m_State);
    

    m_State["Scene"] = m_State.create_table();

    m_State.new_usertype<Scene>("Scene",
        "FindEntityByName", &Scene::GetEntityByName
    );
}