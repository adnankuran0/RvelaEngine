#pragma once
#include "sol/state.hpp"

namespace rv {

class ScriptEngine
{
public:
	void Init();
	sol::state& GetState() { return m_State; }

private:
	void InitOverrides();
	sol::state m_State;
};

}