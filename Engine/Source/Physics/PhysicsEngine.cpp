#include "rvelapch.h"
#include "PhysicsEngine.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include <Jolt/RegisterTypes.h>


rv::PhysicsEngine::PhysicsEngine()
{
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
}

rv::PhysicsEngine::~PhysicsEngine()
{
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

}
