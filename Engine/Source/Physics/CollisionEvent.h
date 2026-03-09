#pragma once
#include "CollisionInfo.h"
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
	CollisionInfo collisionInfo;
	CollisionEventType eventType;
};

}