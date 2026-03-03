#include "rvelapch.h"
#include "BodyActivationListener.h"
#include "Core/Log.h"

using namespace rv::Physics;

void BodyActivationListener::OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
	LOG_DEBUG("A body got activated");
}

void BodyActivationListener::OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData)
{
	LOG_DEBUG("A body went to sleep");
}
