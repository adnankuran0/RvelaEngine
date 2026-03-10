#pragma once
#include "Collision.h"
#include "entt/entt.h"

namespace rv::Physics {

enum class CollisionEventType
{
	ENTER,
	STAY,
	EXIT
};

struct  CollisionEvent
{
	entt::entity entityA;
	entt::entity entityB;
	Collision collision;
	CollisionEventType eventType;
};

}