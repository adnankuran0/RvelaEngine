#include "rvelapch.h"
#include "ScriptEngine.h"
#include "ScriptBindings.h"
#include "sol/variadic_args.hpp"
#include "Core/EditorConsoleSink.h"

using namespace rv;

void ScriptEngine::Init()
{
    m_State.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string, sol::lib::os);

    InitOverrides();

    LuaBindings::RegisterMath(m_State);
    LuaBindings::RegisterCoreTypes(m_State);
    LuaBindings::RegisterComponents(m_State);
    LuaBindings::RegisterInputAPI(m_State);
    LuaBindings::RegisterSceneAPI(m_State);
    LuaBindings::RegisterPhysicsAPI(m_State);
    LuaBindings::RegisterAudioAPI(m_State);
    LuaBindings::RegisterAnimationAPI(m_State);

}

void ScriptEngine::InitOverrides()
{
    m_State["print"] = [](sol::variadic_args va) {
        std::stringstream ss;
        bool first = true;

        for (auto v : va)
        {
            if (!first)
                ss << "\t";

            sol::state_view lua_state = va.lua_state();
            std::string str = lua_state["tostring"](v);
            ss << str;
            first = false;
        }

        std::string output = ss.str();

        std::cout << "[LUA] " << output << std::endl;

        EditorConsoleSink::Get().LogLua(output);
        };
}
