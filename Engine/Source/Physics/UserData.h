#pragma once
#include <cstdint>
#include "entt/entt.h"
#include "CollisionFilter.h"

namespace rv::Physics {
struct UserData
{
    CollisionFilter filter;
    entt::entity entity;
};
}

