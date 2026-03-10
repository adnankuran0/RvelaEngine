#pragma once
#include <cstdint>

namespace rv::Physics {

struct CollisionFilter
{
    uint32_t layer = 1;
    uint32_t mask = 0xFFFFFFFF;
};

}
