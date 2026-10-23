#include "rvelapch.h"
#include "PhysicsEngine.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include <Jolt/RegisterTypes.h>
#include "Core/Log.h"
#include <cstdarg>

using namespace rv;

static void JoltTraceImpl(const char* inFMT, ...)
{
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    LOG_TRACE("{}", buffer);
}

#ifdef JPH_ENABLE_ASSERTS
static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
{
    LOG_ERROR("Jolt Assert failed: {} ({}) at {}:{}", inExpression, inMessage ? inMessage : "", inFile, inLine);
    return true;
}
#endif


rv::PhysicsEngine::PhysicsEngine()
{
    JPH::Trace = JoltTraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailedImpl;)

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
