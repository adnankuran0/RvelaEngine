#pragma once
#include "sol/forward.hpp"


namespace rv::LuaBindings {

class PhysicsSystem;

void RegisterPhysicsAPI(sol::state& lua);

}