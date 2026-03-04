#include "rvelapch.h"
#include "sol/sol.hpp"
#include "PhysicsBindings.h"
#include "Physics/PhysicsSystem.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Physics/PhysicsSystem.h"

using namespace rv;

void LuaBindings::RegisterPhysicsAPI(sol::state& lua)
{

    lua.new_usertype<rv::PhysicsSystem>("Physics",
        "AddForce", &rv::PhysicsSystem::AddForce,
        "AddImpulse", &rv::PhysicsSystem::AddImpulse,
        "SetVelocity", &rv::PhysicsSystem::SetVelocity,
        "GetVelocity", &rv::PhysicsSystem::GetVelocity
    );

}