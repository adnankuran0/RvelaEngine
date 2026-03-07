#include "rvelapch.h"
#include "sol/sol.hpp"
#include "PhysicsBindings.h"
#include "Physics/PhysicsWorld.h"
#include "Scene/Components/RigidbodyComponent.h"

using namespace rv::Physics;

void rv::LuaBindings::RegisterPhysicsAPI(sol::state& lua)
{

    lua.new_usertype<PhysicsWorld>("Physics",
        "AddForce", &PhysicsWorld::AddForce,
        "AddImpulse", &PhysicsWorld::AddImpulse,
        "SetVelocity", &PhysicsWorld::SetVelocity,
        "GetVelocity", &PhysicsWorld::GetVelocity
    );

}