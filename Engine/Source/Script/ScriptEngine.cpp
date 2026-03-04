#include "rvelapch.h"
#include "ScriptEngine.h"
#include "ScriptBindings.h"

using namespace rv;

void ScriptEngine::Init()
{
    m_State.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::os);

    LuaBindings::RegisterMath(m_State);
    LuaBindings::RegisterCoreTypes(m_State);
    LuaBindings::RegisterComponents(m_State);
    LuaBindings::RegisterInputAPI(m_State);
    LuaBindings::RegisterSceneAPI(m_State);
    LuaBindings::RegisterPhysicsAPI(m_State);

    
}