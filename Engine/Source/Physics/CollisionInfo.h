#pragma once
#include "Scene/Entity.h"

namespace rv::Physics {

struct CollisionInfo
{
	Entity other;
	Collision collision;
};


}